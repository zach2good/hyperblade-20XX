"""Fail when a class's declared fields leave a hole or overlap.

tools/check_field_offsets.py checks that the offset comments climb. This asks whether each
comment agrees with where the compiler puts the field. It walks every field in order, adding
each one's width to a running position, and compares that position against the comment. A field
with no comment still takes its bytes.

A run stops at the first field this cannot size and resumes at the next commented offset, so a
class-typed member does not cost the rest of its class.

It cannot see a hole between a base's end and the derived class's first field, since the source
does not say how big the base is. The compile-time assertions in src/common/fieldat.h cover that.

    python tools/check_field_tiling.py
    python tools/check_field_tiling.py --all     every class it could measure

Exit code 1 when a class has a hole or an overlap between two fields it can size.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

CLASS = re.compile(r"^(?:class|struct) ([0-9A-Za-z_]+)(?:\s*:\s*public [0-9A-Za-z_]+)?\s*"
                   + chr(10) + r"\s*\{(.*?)" + chr(10) + r"\};", re.S | re.M)

FIELD = re.compile(r"^[ \t]+(?!virtual|typedef)"
                   + r"((?:(?!p(?:ublic|rotected|rivate)\s*:)[\w:*&<>])[\w:*&<>\s]*?)"
                   + r"[\s*]([0-9A-Za-z_]+)((?:\[[^\]]*\])*)?;[ \t]*//\s*(0x[0-9a-fA-F]+)", re.M)

SCALAR = {"char": 1, "unsignedchar": 1, "signedchar": 1, "BYTE": 1,
          "short": 2, "unsignedshort": 2, "WORD": 2,
          "int": 4, "unsignedint": 4, "long": 4, "unsignedlong": 4, "float": 4,
          "DWORD": 4, "UINT": 4, "HRESULT": 4, "BOOL": 4, "LONG": 4,
          "double": 8}

# Sizes the scalar table cannot know are read out of the tree rather than listed here.
WINDOWS = {"RECT": 16, "POINT": 8, "HRESULT": 4, "BOOL": 4, "LONG": 4}

EXTENT = re.compile(r"\[([^\]]*)\]")

UNION = re.compile(r"^[ 	]*union[ 	]*" + chr(10) + r"[ 	]*\{", re.M)

# Every field, with or without an offset comment: a reserve written without one still takes its
# bytes.
ALL = re.compile(r"^[ 	]+(?!virtual|typedef|public|protected|private)"
                 + r"((?:(?!p(?:ublic|rotected|rivate)\s*:)[\w:*&<>])[\w:*&<>\s]*?)"
                 + r"[\s*]([0-9A-Za-z_]+)((?:\[[^\]]*\])*)?;[ 	]*(?://\s*(0x[0-9a-fA-F]+))?",
                 re.M)


def sourceNames(root):
    # Walk subdirectories; names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def withoutUnions(body):
    # A union is one field. Each union becomes one member this cannot size, which ends the run
    # and resumes it at the next commented offset.
    out = []
    at = 0

    while True:
        found = UNION.search(body, at)

        if not found:
            out.append(body[at:])

            return "".join(out)

        out.append(body[at:found.start()])
        out.append("    UnionRun unionRun;" + chr(10))
        depth = 0
        position = found.end() - 1

        while position < len(body):
            if body[position] == "{":
                depth += 1
            elif body[position] == "}":
                depth -= 1

                if depth == 0:
                    break

            position += 1

        semicolon = body.find(";", position)
        at = len(body) if semicolon < 0 else semicolon + 1


def width(kind, arrays):
    kind = kind.replace(" ", "").replace("class", "").replace("struct", "")

    if "*" in kind:
        size = 4
    elif kind in SCALAR:
        size = SCALAR[kind]
    elif kind in WINDOWS:
        size = WINDOWS[kind]
    else:
        # A member whose type is one of this tree's own classes ends the run: sizing it would
        # mean modelling its vptr, alignment and every member in turn.
        return None

    for extent in EXTENT.findall(arrays or ""):
        try:
            size = size * int(extent, 0)
        except ValueError:
            return None

    return size


def sources():
    for name in sorted(sourceNames(SRC)):
        if name.endswith((".cpp", ".h")):
            text = open(os.path.join(SRC, name), encoding="utf-8", errors="replace").read()

            yield name, text.replace(chr(13) + chr(10), chr(10))


def main():
    faults = []
    measured = 0
    seen = set()
    for name, text in sources():
        for match in CLASS.finditer(text):
            klass = match.group(1)

            if klass in seen:
                continue

            seen.add(klass)
            rows = ALL.findall(withoutUnions(match.group(2)))

            if len([1 for row in rows if row[3]]) < 2:
                continue

            measured += 1
            at = None

            for kind, field, arrays, offset in rows:
                size = width(kind, arrays)

                # VC6 aligns a field to its own width capped at four, but records read straight
                # out of a file or a packet are packed. A comment that agrees with either packing
                # is accepted, because nothing here can tell the two apart.
                packed = at
                aligned = at

                if size is not None and at is not None and min(size, 4) > 1:
                    align = min(size, 4)
                    aligned = (at + align - 1) & ~(align - 1)

                # Every class this tree embeds holds a dword, so it starts on a four-byte boundary.
                if size is None and at is not None:
                    aligned = (at + 3) & ~3

                if offset:
                    claimed = int(offset, 16)

                    if at is not None and claimed != packed and claimed != aligned:
                        faults.append((name, klass, field, claimed, packed))

                    at = claimed

                if size is None:
                    at = None
                elif at is not None:
                    at = at + size

    for name, klass, field, claimed, at in faults:
        print("%s  %s: %s says 0x%03x, the fields before it reach 0x%03x, %s of 0x%x"
              % (name, klass, field, claimed, at,
                 "a hole" if claimed > at else "an overlap", abs(claimed - at)))

    print("%d class(es) measured, %d with a hole or an overlap" % (measured, len(faults)))

    if faults:
        print("Describe the bytes between them, or correct the width of the field before.")

        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
