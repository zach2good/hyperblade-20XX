"""Find reccmp markers whose name is not the declaration on the line under them.

reccmp reads a marker's name from the line immediately after it and substitutes that name into
the original's disassembly; the recomp side takes the name from the PDB. The two are compared as
text, so a comment between a marker and its declaration becomes the original's name for that
symbol, and every instruction touching it stops matching.

It also reports a marker's text anywhere other than alone on its own comment line, where
reccmp does not see it at all.
"""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

MARKER = re.compile(r"^\s*//\s*(FUNCTION|GLOBAL|STRING|VTABLE|SYNTHETIC|LIBRARY|TEMPLATE)"
                    r":\s+\w+\s+0x[0-9a-fA-F]+\s*$")


# A marker's text anywhere other than alone on its own comment line. A rewrap can fold a marker
# into the prose above it, and reccmp then treats the address as not implemented.
BURIED = re.compile(r"HYPERX\s+0x[0-9a-fA-F]+")


def sourceNames(root):
    # Paths come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def main():
    rows = []
    buried = []

    for entry in sorted(sourceNames(SRC)):
        if not entry.endswith((".cpp", ".h")):
            continue

        path = os.path.join(SRC, entry)
        lines = open(path, encoding="utf-8", errors="replace").read().split("\n")

        for i, line in enumerate(lines):
            if BURIED.search(line) and not MARKER.match(line.rstrip()):
                if line.lstrip().startswith("//"):
                    buried.append((entry, i + 1, line.strip()[:76]))

            if not MARKER.match(line.rstrip()):
                continue

            # A marker may be followed by another marker; the name comes from the first line
            # under the run that is not one.
            j = i + 1

            while j < len(lines) and MARKER.match(lines[j].rstrip()):
                j += 1

            if j >= len(lines):
                rows.append((entry, i + 1, line.strip(), "(end of file)"))
                continue

            under = lines[j].strip()

            if not under.startswith("//"):
                continue

            # In src/common/crtglobals.h a comment under a marker is the name, because the symbol
            # has no C declaration. The fault is a comment with a declaration under it: the
            # declaration then names the recomp side and the comment names the original.
            k = j + 1

            while k < len(lines) and lines[k].strip().startswith("//"):
                k += 1

            if k >= len(lines):
                continue

            follows = lines[k].strip()

            if follows and not follows.startswith("//") and follows != "}":
                rows.append((entry, i + 1, line.strip(), follows[:56]))

    for entry, number, marker, under in rows:
        print("NAMED BY A COMMENT  %-26s %5d  %-34s %s" % (entry, number, marker, under))

    for entry, number, line in buried:
        print("BURIED IN PROSE     %-26s %5d  %s" % (entry, number, line))

    print("%d marker(s) whose name is a comment or a blank line, %d buried in prose"
          % (len(rows), len(buried)))

    if rows or buried:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
