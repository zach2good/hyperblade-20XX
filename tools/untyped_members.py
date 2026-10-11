# Report the types the tree casts each `void*` and `char*` member to.
#
# The cast at a use site is the evidence for what the member is. Each member lands in one bucket:
#
#   ONE      every cast agrees. Retype the member and the casts go.
#   BASE     the cast types share one of their own number as an ancestor. A candidate to
#            build, since only the compiler can confirm it.
#   SEVERAL  the casts disagree, which usually means the field really holds different types.
#            These want a note in the header, not a type.
#   NOTED    one cast and a note in the header saying why the member stays as it is.
#   READ     no cast, and the body dereferences it as the char* it is declared.
#   NONE     nothing casts or dereferences it. The assignments into it are listed, since they
#            are the only evidence.
#
#   python tools/untyped_members.py            # the summary line
#   python tools/untyped_members.py --list     # every member with its bucket
#
# Read only. An assert checks that every member reaches a bucket.

import collections
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

MEMBER = re.compile(r"^\s*(void|char)\*\s+(m_\w+)\s*;", re.M)
# Any pointer declaration of the name, whatever its type. The owner count uses these, since two
# classes can declare the same name with different types.
ANY = re.compile(r"^\s*(?:const\s+)?(?:class\s+|struct\s+)?[\w:]+\s*\*+\s*(m_\w+)\s*;", re.M)
# The two shapes a use site takes: a cast of the member, or a cast of its address.
CAST = r"\(\s*(?:const\s+)?(?:class\s+|struct\s+)?(\w+)\s*\*+\s*\)\s*(?:\(\s*)?[\w>.\-]*\b%s\b"
ADDR = r"\(\s*(?:const\s+)?(?:class\s+|struct\s+)?(\w+)\s*\*+\s*\)\s*&[\w>.\-]*\b%s\b"
DEREF = r"\b%s\s*(?:->|\[)"
# Byte arithmetic on the member. A retype silently rescales every such step by the new element
# size, so a stepped member is never offered as a plain retype.
ARITH = r"\b%s\s*(?:\+\+|--|\+=|-=)|\b%s\s*[+-]\s*[\w(]"
# An assignment into the member. A zero is skipped: constructors clear every one of these.
ASSIGN = re.compile(r"\b(m_\w+)\s*=\s*([^=;][^;]*)")

SKIP = ("void", "char", "int", "short", "long", "unsigned", "float", "double", "const")

# When the cast types share one of their own number as an ancestor, the base can carry every
# reading and the rest become downcasts. This is reported and not applied: a site may assign an
# unrelated type, and only the compiler finds that.
BASE = re.compile(r"^\s*class\s+(\w+)\s*:\s*(?:public\s+|protected\s+|private\s+)?(\w+)",
                  re.M)


def parents(root):
    """Each class mapped to the one it derives from, over the whole tree."""
    found = {}

    for path in sources(root):
        if not path.endswith(".h"):
            continue

        text = open(path, encoding="utf-8", errors="replace").read()

        for child, parent in BASE.findall(code(text)):
            found.setdefault(child, parent)

    return found


def commonBase(kinds, parent):
    """The one of these types every other derives from, or "" if there is none."""
    for candidate in kinds:
        ok = True

        for kind in kinds:
            at = kind
            seen = 0

            while at != candidate and at in parent and seen < 16:
                at = parent[at]
                seen += 1

            if at != candidate:
                ok = False
                break

        if ok:
            return candidate

    return ""


def sources(root):
    found = []

    for directory, unused, names in os.walk(root):
        for name in sorted(names):
            if name.endswith((".cpp", ".h")):
                found.append(os.path.join(directory, name))

    return found


def collect(root):
    files = sources(root)
    bodies = {}
    declared = collections.defaultdict(list)
    owners = collections.Counter()

    for path in files:
        text = open(path, encoding="utf-8", errors="replace").read()
        bodies[path] = code(text)

        if path.endswith(".h"):
            for name in set(ANY.findall(bodies[path])):
                owners[name] += 1

            for kind, name in MEMBER.findall(bodies[path]):
                shown = os.path.relpath(path, os.path.dirname(root)).replace(os.sep, "/")
                declared[name].append((shown, kind, text))

    found = {}

    for name in declared:
        casts = collections.Counter()
        deref = 0
        uses = 0
        steps = 0

        for path in files:
            body = bodies[path]

            if name not in body:
                continue

            uses += len(re.findall(r"\b%s\b" % name, body))

            # A cast of the address and a cast of the value both count. The value form is
            # matched with the address form removed, so one site cannot count twice.
            for hit in re.findall(ADDR % re.escape(name), body):
                if hit not in SKIP:
                    casts[hit] += 1

            for hit in re.findall(CAST % re.escape(name), body.replace("&" + name, " ")):
                if hit not in SKIP:
                    casts[hit] += 1

            deref += len(re.findall(DEREF % re.escape(name), body))
            steps += len(re.findall(ARITH % (re.escape(name), re.escape(name)), body))

        found[name] = (casts, deref, uses, steps)

    return declared, found, owners


def writeSites(root, declared):
    """Every assignment INTO one of these members, as file:line and the right hand side."""
    found = collections.defaultdict(list)

    for path in sources(root):
        if not path.endswith(".cpp"):
            continue

        shown = os.path.relpath(path, os.path.dirname(root)).replace(os.sep, "/")
        text = open(path, encoding="utf-8", errors="replace").read()

        for at, line in enumerate(code(text).split("\n"), 1):
            for hit in ASSIGN.finditer(line):
                name = hit.group(1)

                if name not in declared:
                    continue

                rhs = hit.group(2).strip()

                if rhs in ("0", "0;"):
                    continue

                found[name].append("%s:%d  %s" % (shown, at, rhs[:64]))

    return found


def noted(text, name):
    """The comment block above the declaration, if it says anything."""
    at = text.find(name)

    if at < 0:
        return ""

    lines = text[:at].split("\n")
    kept = []

    for line in reversed(lines[:-1]):
        if not line.strip().startswith("//"):
            break

        kept.append(line.strip()[2:].strip())

    return " ".join(reversed(kept))


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    declared, found, owners = collect("src")
    parent = parents("src")
    base = []
    writes = writeSites("src", declared)
    one = []
    several = []
    none = []
    settled = []
    read = []

    for name in sorted(declared):
        casts, deref, uses, steps = found[name]
        where = ", ".join(path for path, unused, unused2 in declared[name])
        note = noted(declared[name][0][2], name)

        # Casts are gathered by name, so a name declared in several headers pools several
        # fields. Only a name one header declares can be retyped from this.
        if len(casts) == 1 and not note and owners[name] == 1 and not steps:
            one.append((name, where, list(casts)[0], casts[list(casts)[0]]))
        elif len(casts) == 1 and not note and owners[name] == 1:
            several.append((name, where, casts, "stepped in bytes at %d site(s)" % steps))
        elif len(casts) == 1 and not note:
            several.append((name, where, casts, "declared in %d headers" % owners[name]))
        elif len(casts) > 1:
            shared = commonBase(list(casts), parent)

            if shared and not note and owners[name] == 1 and not steps:
                base.append((name, where, casts, shared))
            else:
                several.append((name, where, casts, note))
        elif len(casts) == 1:
            settled.append((name, where, list(casts)[0], note))
        elif not casts and deref:
            read.append((name, where, deref, declared[name][0][1]))
        elif not casts and not deref:
            # The declared kind splits this bucket. A char* nothing casts may already be right;
            # a void* is what the decompiler writes when it could not tell.
            none.append((name, where, uses, note, declared[name][0][1]))

    if show:
        for name, where, kind, n in one:
            print("ONE      %-24s %-40s %s, %d site(s)" % (name, where, kind, n))
        for name, where, casts, shared in base:
            print("BASE     %-24s %-40s %s covers %s"
                  % (name, where, shared,
                     ", ".join("%s(%d)" % (k, v) for k, v in casts.most_common())))
        for name, where, casts, note in several:
            print("SEVERAL  %-24s %-40s %s%s"
                  % (name, where, ", ".join("%s(%d)" % (k, v) for k, v in casts.most_common()),
                     "   NOTED" if note else ""))
        for name, where, kind, note in settled:
            print("NOTED    %-24s %-40s %s, settled beside it" % (name, where, kind))
        for name, where, deref, kind in read:
            print("READ     %-24s %-40s %s*, %d dereference(s) and no cast"
                  % (name, where, kind, deref))
        for name, where, uses, note, kind in sorted(none, key=lambda row: row[4]):
            print("NONE %-5s %-24s %-40s %d mention(s)%s"
                  % (kind + "*", name, where, uses, "   NOTED" if note else ""))

            for site in writes.get(name, [])[:4]:
                print("           writes  %s" % site)

    voids = [row for row in none if row[4] == "void"]

    # A note beside a void* settles it as much as one beside a cast.
    unnoted = [row for row in voids if not row[3]]

    assert (len(one) + len(base) + len(several) + len(none) + len(settled) + len(read)
            == len(declared)), "a member reached no bucket"

    print("%d untyped member(s): %d cast to one type everywhere, %d to a set with a common "
          "base among them, %d to several, %d settled by a note beside them, %d dereferenced "
          "as what they are declared, %d never cast or dereferenced of which %d are "
          "void* and undecoded by construction, %d of those with no note"
          % (len(declared), len(one), len(base), len(several), len(settled), len(read),
             len(none), len(voids), len(unnoted)))

    return 0


if __name__ == "__main__":
    sys.exit(main())
