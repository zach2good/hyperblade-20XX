# Lists members whose name gives only their offset, ranked by how many use sites they have.
#
#   python tools/placeholder_fields.py          # the summary line
#   python tools/placeholder_fields.py --list   # one row per field
#
# Each field is classed:
#
#   OPEN     a site says something about it and nothing records a reason.
#   NOTED    the declaration or a use site carries a note about it.
#   MUTE     every site is a constructor zero, a literal store or a copy of itself.
#   POOLED   more than one class declares the name, so it is not one field.
#
# A field that is both mute and noted reads as mute. A store is not evidence, so stores do not
# make a site speak. A note counts only when it sits directly above the declaration, beside it,
# or names the field or its offset; a function body's comments are usually about the code.
# Ownership is by class, not by file, since one header can declare several classes with the same
# field name.
#
# Read only.

import collections
import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

DECL = re.compile(r"^[ \t]*(?:const\s+|unsigned\s+|signed\s+|struct\s+|class\s+)*[\w:]+\**\s+"
                  r"(m_\w*[Ff]ield[0-9a-fA-F]+\w*|m_\w*[Ff]ield\d+\w*)\s*[;\[]", re.M)

# A definition and not a forward declaration, at column nought so that a member such as
# `class EngineObject* m_pOwner;` is not read as an opener. A `__declspec(...)` attribute is
# skipped so the class name is captured.
OPENER = re.compile(r"^(?:class|struct)\s+(?:__declspec\s*\([^)]*\)\s*)?"
                    r"(\w+)\b[^;\r\n]*$", re.M)

# What a trailing comment has to say beyond the offset the declaration already carries.
BARE = re.compile(r"^(?:0x[0-9a-fA-F]+|[-,.;:]|\s)*")

# A site that says nothing: an initialiser list entry, a store of a literal or named constant,
# or a copy of the same name off another object. A store through a pun, such as
# `*(int*)&m_aField06c[2] = g_nZeroEngineObject;`, is still a store.
TARGET = r"(?:\*\(\s*\w+\s*\*\s*\)\s*&\s*)?%s(?:\[\s*\w+\s*\])?"
MUTE = (r"^[:,]?\s*%s\s*\(\s*[-\w.]+\s*\)\s*,?$",
        r"^" + TARGET + r"\s*=\s*[-\w.]+\s*;$",
        r"^" + TARGET + r"\s*=\s*[\w>.*()-]*\b%s\s*;$")


def noteAbove(raw, at, skip=0):
    """The comment block above an offset, as one string.

    `skip` allows that many non-comment lines between the offset and the block, so a use site
    inside a function can reach a note above the function's signature.
    """
    kept = []
    allowed = skip

    for line in reversed(raw[:at].rstrip().split("\n")):
        if not line.strip().startswith("//"):
            if kept or allowed <= 0:
                break

            allowed = allowed - 1
            continue

        kept.append(line.strip()[2:].strip())

    note = " ".join(reversed(kept))

    # Marker lines such as `// FUNCTION: HYPERX 0x...` are not notes.
    note = re.sub(r"(?:FUNCTION|GLOBAL|STRING|VTABLE|SYNTHETIC|LIBRARY|STUB|SLOT TABLE):"
                  r"\s*\w*\s*0x[0-9a-fA-F]+", " ", note)

    return note.strip() if len(note.strip()) > 24 else ""


def owner(opens, at):
    """The class whose body an offset falls in, by the last opener above it."""
    here = ""

    for start, name in opens:
        if start > at:
            break

        here = name

    return here or "(file scope)"


def about(name, note):
    """Whether a note names the field it was found near, by name or by offset."""
    if name in note:
        return True

    hexes = re.search(r"[Ff]ield([0-9a-fA-F]+)", name)

    if not hexes:
        return False

    return re.search(r"0x0*%s\b" % hexes.group(1).lstrip("0"), note, re.I) is not None


def speaks(name, text, at):
    """Whether the site at an offset says anything the name could be read from."""
    line = text[text.rfind("\n", 0, at) + 1:].split("\n")[0].strip()
    quoted = re.escape(name)

    for shape in MUTE:
        if re.match(shape % ((quoted,) * shape.count("%s")), line):
            return False

    return True


def noteBeside(raw, at):
    """The trailing comment on the declaration's own line, with a bare offset stripped."""
    line = raw[at:].split("\n")[0]
    mark = line.find("//")

    if mark < 0:
        return ""

    note = BARE.sub("", line[mark + 2:].strip()).strip()

    return note if len(note) > 3 else ""


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    headers = sorted(glob.glob(os.path.join("src", "**", "*.h"), recursive=True))
    sources = sorted(glob.glob(os.path.join("src", "**", "*.cpp"), recursive=True))

    owners = collections.defaultdict(set)
    notes = collections.defaultdict(str)

    for path in headers:
        shown = os.path.relpath(path, "src").replace(os.sep, "/")
        raw = open(path, encoding="utf-8", errors="replace").read()
        text = code(raw)

        opens = [(hit.start(), hit.group(1)) for hit in OPENER.finditer(text)]

        for hit in DECL.finditer(text):
            owners[hit.group(1)].add("%s::%s" % (shown, owner(opens, hit.start())))

            if not notes[hit.group(1)]:
                name = hit.group(1)
                note = noteAbove(raw, hit.start()) or noteBeside(raw, hit.start())

                if not note:
                    # A note above a run of declarations can sit several lines above the last of
                    # them; about() still requires it to name this field.
                    shared = noteAbove(raw, hit.start(), skip=12)

                    if about(name, shared):
                        note = shared

                notes[name] = note

    uses = collections.Counter()
    said = collections.Counter()

    for path in sources:
        raw = open(path, encoding="utf-8", errors="replace").read()
        text = code(raw)

        for name in owners:
            for hit in re.finditer(r"\b%s\b" % re.escape(name), text):
                uses[name] += 1

                if speaks(name, text, hit.start()):
                    said[name] += 1

                # A note above any use site answers for the field if it names the field.
                if not notes[name]:
                    found = noteAbove(raw, hit.start(), skip=3)

                    if about(name, found):
                        notes[name] = found

    rows = []

    for name in owners:
        rows.append((uses[name], name, sorted(owners[name]), bool(notes[name]),
                     said[name] > 0))

    rows.sort(reverse=True)

    pooled = [row for row in rows if len(row[2]) > 1]
    single = [row for row in rows if len(row[2]) == 1]
    mute = [row for row in single if not row[4]]
    openRows = [row for row in single if row[4] and not row[3]]

    if show:
        for count, name, where, note, says in single:
            print("%-7s %-24s %-46s %d use(s)"
                  % ("MUTE" if not says else "NOTED" if note else "OPEN",
                     name, where[0][:46], count))

        for count, name, where, note, says in pooled:
            print("POOLED  %-24s %-46s %d use(s), %d classes"
                  % (name, " ".join(w.split("::")[-1] for w in where)[:46], count, len(where)))

    print("%d placeholder field name(s): %d open, %d noted, %d mute, "
          "%d declared by more than one class"
          % (len(rows), len(openRows), len(single) - len(openRows) - len(mute),
             len(mute), len(pooled)))

    return 0


if __name__ == "__main__":
    sys.exit(main())
