"""Split raw-offset `char*` locals into those a class already covers and those that need a decode.

A local counts as CONVERTIBLE when a class it can be tied to covers at least half of its
offsets. The class comes from a cast of the local, from `(char*)this` inside a member, or from
the declaration or a cast of the pointer it was flattened from (one hop only). PARTIAL rows have
a class that covers less than half; DECODE FIRST rows have no class, or one that covers nothing.

    python tools/blob_locals.py                        # the split
    python tools/blob_locals.py --blocked              # list DECODE FIRST too, biggest first
    python tools/blob_locals.py --blocked --limit 80   # print 80 rows per tier (default 12)
    python tools/blob_locals.py --local pEngine        # one local name only

CONVERTIBLE is about types, not about whether converting is wise. Many rows sit in functions
that already match exactly, or in files whose notes record that the raw-offset spelling is what
matches. Read the note beside a row before taking it. A `KEEP.` comment directly above the
declaration drops the row.

A row is one name per file. When more than one function flattens `this` into the same name,
the row is POOLED and gets no class, because its offsets belong to several classes.

`sizeof(EngineObject) == 0xd14`, so every derived class puts its first field there. An offset
above that size identifies no class on its own, and a `GetSlot()` guard tests a runtime field,
not the type. Settle such a row with a traced run: HB_TRACE_POS logs each live object's vtable,
and build-bringup/out/HYPERX.map maps it back to its class.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import self_fields
from comments import code

SRC = self_fields.SRC
LINE = chr(10)

DECLARE = r"(?<![0-9A-Za-z_])char\*\s+(?:const\s+)?%s\s*[;=]"


def sourceNames(root):
    # Sources live in subdirectories, so walk rather than list. Names are relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


KEEP = "KEEP."


def kept(text, local):
    """Whether the comment block directly above the local's declaration contains `KEEP.`.

    Anchored on the declaration, so a mention of the local elsewhere cannot silence a row.
    """
    hit = re.search(DECLARE % re.escape(local), text)

    if hit is None:
        return False

    lines = text[:hit.start()].split(LINE)
    block = []

    for line in reversed(lines[:-1]):
        if not line.strip().startswith("//"):
            break

        block.append(line)

    return any(KEEP in line for line in block)


def offsets(text, local, macros):
    """Every constant the local is given, resolved through the file's own defines."""
    found = []

    for hit in re.finditer(r"(?<![0-9A-Za-z_])%s \+ ([0-9A-Za-zx_]+)" % re.escape(local), text):
        word = hit.group(1)

        if word.startswith("0x"):
            found.append(int(word, 16))
        elif word in macros:
            found.append(macros[word])

    return found


def claimed(text, local):
    """The classes the file casts this local to, which is what it says the thing is."""
    return set(re.findall(r"\(\s*([A-Za-z_][0-9A-Za-z_]*)\s*\*\s*\)\s*%s(?![0-9A-Za-z_])"
                          % re.escape(local), text)) - set(["char", "void", "int", "float",
                                                            "short", "unsigned"])


# A member definition, so the class name can be recovered from the text above a use.
MEMBER = re.compile(r"^[A-Za-z_][0-9A-Za-z_:<>*&\s]*?([A-Za-z_][0-9A-Za-z_]*)::~?[A-Za-z_]"
                    r"[0-9A-Za-z_]*\s*\(", re.M)


def pooled(text, local):
    """How many separate functions in this file flatten `this` into the same local name."""
    return len(re.findall(r"(?<![0-9A-Za-z_])%s\s*=\s*\(\s*char\s*\*\s*\)\s*this\s*;"
                          % re.escape(local), text))


def owner(text, local):
    """The class of a local assigned `(char*)this`, read from the enclosing member's name."""
    hit = re.search(r"(?<![0-9A-Za-z_])%s\s*=\s*\(\s*char\s*\*\s*\)\s*this\s*;"
                    % re.escape(local), text)

    if hit is not None:
        above = [m.group(1) for m in MEMBER.finditer(text, 0, hit.start())]

        if above:
            return set([above[-1]])

    return set()


def flattened(text, local):
    """The class of a local assigned `(char*)pSomethingTyped`, from that source's declaration."""
    hit = re.search(r"(?<![0-9A-Za-z_])%s\s*=\s*\(\s*char\s*\*\s*\)\s*"
                    r"([A-Za-z_][0-9A-Za-z_]*)\s*;" % re.escape(local), text)

    if hit is None:
        return set()

    source = hit.group(1)
    found = re.findall(r"(?<![0-9A-Za-z_])([A-Za-z_][0-9A-Za-z_]*)\s*\*\s*(?:const\s+)?%s"
                       r"(?![0-9A-Za-z_])" % re.escape(source), text)

    # The source is often a void* whose class comes from a cast elsewhere in the file. One hop
    # is enough: every source in this tree is a parameter or a `void*` flattened once.
    return ((set(found) | claimed(text, source))
            - set(["char", "void", "int", "float", "short", "unsigned", "const"]))


def main():
    wanted = None
    show_blocked = "--blocked" in sys.argv[1:]
    limit = 12

    if "--limit" in sys.argv[1:]:
        limit = int(sys.argv[sys.argv.index("--limit") + 1])

    if "--local" in sys.argv[1:]:
        wanted = sys.argv[sys.argv.index("--local") + 1]

    known = self_fields.records()
    layouts = {}
    rows = []

    for name in sorted(sourceNames(SRC)):
        if not name.endswith(".cpp"):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8",
                       errors="replace").read().replace(chr(13) + LINE, LINE)

        # Scans run on masked text so code quoted in a comment is not read as a declaration.
        # kept() alone reads the raw text, because it looks for notes.
        blind = code(text)
        macros = dict((key, int(value, 16))
                      for key, value in self_fields.DEFINE.findall(blind))

        for local in sorted(set(re.findall(r"(?<![0-9A-Za-z_])char\*\s+(?:const\s+)?"
                                           r"([0-9A-Za-z_]+)\s*[;=]", blind))):
            if wanted and local != wanted:
                continue

            # A row already read and declined carries `KEEP.` and a reason above its declaration.
            if kept(text, local):
                continue

            used = offsets(blind, local, macros)

            if not used:
                continue

            # Several functions flattening `this` into this name mix several classes' offsets.
            if pooled(blind, local) > 1:
                says = set()
            else:
                says = (claimed(blind, local) | owner(blind, local)
                        | flattened(blind, local))

            best = None

            for klass in says:
                if klass not in layouts:
                    layouts[klass] = self_fields.layout(klass, known)

                covered = sum(1 for offset in used if offset in layouts[klass])

                if best is None or covered > best[1]:
                    best = (klass, covered)

            rows.append((name, local, len(used), best, says))

    convertible = [r for r in rows if r[3] and r[3][1] * 2 >= r[2]]
    partial = [r for r in rows if r[3] and 0 < r[3][1] * 2 < r[2]]
    blocked = [r for r in rows if not r[3] or r[3][1] == 0]

    for title, group in (("CONVERTIBLE", convertible), ("PARTIAL", partial),
                         ("DECODE FIRST", blocked)):
        print("%-13s %3d local(s), %4d offset(s)"
              % (title, len(group), sum(row[2] for row in group)))

        if wanted or show_blocked or title != "DECODE FIRST":
            for name, local, count, best, found_class in sorted(group,
                                                                key=lambda row: -row[2])[:limit]:
                if best:
                    says = "%s covers %d" % best
                elif found_class:
                    says = "%s covers none of them" % ", ".join(sorted(found_class))
                elif pooled(open(os.path.join(SRC, name), encoding="utf-8",
                                    errors="replace").read(), local) > 1:
                    says = "POOLED across several classes; not one local"
                else:
                    says = "no class named anywhere"
                print("    %-28s %-14s %3d offset(s)  %s" % (name, local, count, says))

    return 0


if __name__ == "__main__":
    sys.exit(main())
