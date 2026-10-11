# Counts pointer casts applied to pointer casts.
#
#     python tools/check_chained_casts.py          # the counts
#     python tools/check_chained_casts.py --list   # every site
#
# Four shapes, counted apart:
#
#   ADJACENT       (T*)(U*)x         Two casts where one would do.
#   VIA INT        (T*)(int)x        A pointer laundered through an integer, which defeats
#                                    every type check.
#   NESTED         *(T**)(*(U**)y)   A deref-cast of a deref-cast. Fine while the record is a
#                                    blob; each one is a decode still to do.
#   CAST OF DEREF  (T**)*(U**)&x     A cast applied to a dereferenced cast. The `&` is required
#                                    and volatile is excluded, which keeps it off a pointer
#                                    read out of a blob, `(T*)*(U**)(p + OFF)`, and a measured
#                                    reload, `(int*)*(volatile int*)&n`. What remains is a cast
#                                    to delete, or a field declared `int` that every reader
#                                    puns to a pointer and should be retyped.
#
# A cast to a function pointer, `(void (__fastcall*)(void*))`, is one cast.
#
# Always exits 0: the counts are a work queue for tools/status.py, not a fault.

import os
import re
import sys

TYPE = r"(?:const\s+|volatile\s+|struct\s+|class\s+|unsigned\s+|signed\s+)*[A-Za-z_]\w*"

ADJACENT = re.compile(r"\(\s*(%s)\s*\*+\s*\)\s*\(\s*(%s)\s*\*+\s*\)" % (TYPE, TYPE))
VIA_INT = re.compile(r"\(\s*(%s)\s*\*+\s*\)\s*\(\s*(?:unsigned\s+|signed\s+)?"
                     r"(?:int|long|short|__int32)\s*\)" % TYPE)
NESTED = re.compile(r"\*\s*\(\s*%s\s*\*\*+\s*\)\s*\(\s*\*\s*\(" % TYPE)

CAST_OF_DEREF = re.compile(r"\(\s*(%s)\s*\*+\s*\)\s*\*\s*\(\s*(?!volatile)(%s)\s*\*+\s*\)\s*&" % (TYPE, TYPE))

CONVENTION = ("__fastcall", "__stdcall", "__cdecl", "__thiscall")


def sourceNames(root):
    # Sources live in subdirectories, so walk rather than list. Names are relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def scan(path):
    lines = open(path, encoding="utf-8", errors="replace").read().split("\n")
    found = []

    for n, line in enumerate(lines, 1):
        if line.lstrip().startswith("//"):
            continue

        for m in ADJACENT.finditer(line):
            if m.group(1) in CONVENTION or m.group(2) in CONVENTION:
                continue
            found.append(("adjacent", n, line.strip()))

        for m in VIA_INT.finditer(line):
            if m.group(1) in CONVENTION:
                continue
            found.append(("via int", n, line.strip()))

        for m in CAST_OF_DEREF.finditer(line):
            if m.group(1) in CONVENTION or m.group(2) in CONVENTION:
                continue
            found.append(("cast of deref", n, line.strip()))

        for m in NESTED.finditer(line):
            found.append(("nested", n, line.strip()))

    return found


def main():
    root = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "src")
    show = "--list" in sys.argv

    tally = {"adjacent": 0, "via int": 0, "nested": 0, "cast of deref": 0}
    files = set()

    for name in sorted(sourceNames(root)):
        if not name.endswith((".cpp", ".h")):
            continue

        for kind, n, text in scan(os.path.join(root, name)):
            tally[kind] += 1
            files.add(name)

            if show:
                print("src/%s:%d: %s" % (name, n, kind))
                print("    %s" % text[:120])

    total = sum(tally.values())

    print("%d chained pointer cast(s) in %d file(s): %d adjacent, %d via int, %d nested, "
          "%d cast of deref"
          % (total, len(files), tally["adjacent"], tally["via int"], tally["nested"],
             tally["cast of deref"]))

    # A work queue, not a fault. tools/status.py reads the last line.
    return 0


if __name__ == "__main__":
    sys.exit(main())
