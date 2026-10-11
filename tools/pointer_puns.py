# Counts `*(int*)&X` puns where X is a pointer or a four byte integer, the cases that are cruft.
#
# Usage: python tools/pointer_puns.py [--list]
#   --list  print each POINTER and SCALAR site.
# Read only; always exits 0.
#
# The decompiler writes `*(int*)&X` wherever it could not see X's type. Each pun is sorted by
# the declared type of what it lands on:
#
#   POINTER   a name declared as a pointer, with no member access after it. The cast comes out.
#   REACHED   the expression walks through `.`, `->` or `[]` first, usually to a float or a
#             matrix cell. Keep: an integer store is not an FPU one.
#   SCALAR    a four byte integer. The cast changes only signedness, but a shift differs
#             (p[1] >> 16 is SAR where the pun is SHR), so sweep these by hand.
#   NARROW    a name shorter than four bytes, or a float. Keep: an int store over two shorts
#             writes both at once, and retyping it would write past the field.
#   OTHER     a name no header declares, such as a local or a file static.

import collections
import os
import re
import sys

import comments

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

PUN = re.compile(r"\*\(\s*int\s*\*\s*\)&\s*([A-Za-z_][\w>.\[\]\-]*)")
# One star or several, so MeshBone** counts as a pointer.
MEMBER = re.compile(r"^\s*(?:const\s+|volatile\s+|mutable\s+|struct\s+|class\s+)*"
                    r"([\w:]+)\s*\*+\s*(\w+)\s*;", re.M)
REACHES = ("->", ".", "[")
# A four byte integer member, where the cast is signedness and nothing else.
WIDE = re.compile(r"^\s*(?:const\s+|volatile\s+|mutable\s+)*"
                 r"((?:unsigned\s+|signed\s+)?(?:int|long|DWORD|BOOL|HRESULT))\s+(\w+)\s*;",
                 re.M)
# A member narrower than the cast, which the pun deliberately reaches past.
NARROW = re.compile(r"^\s*(?:const\s+|volatile\s+|mutable\s+)*"
                   r"((?:unsigned\s+|signed\s+)?(?:short|char)|WORD|BYTE|POINT|Point)\s+(\w+)\s*;",
                   re.M)
# A float, a vector or a matrix cell, where an integer store is not an FPU one.
FLOATY = re.compile(r"^\s*(?:const\s+|volatile\s+|mutable\s+|struct\s+|class\s+)*"
                   r"(float|double|Vec2|Vec3|Vec4|Matrix)\s+(\w+)\s*"
                   r"(?:\[[^\]]*\])?\s*;", re.M)


def namesOf(root, pattern):
    """Every member name a header declares under one of these types."""
    found = set()

    for path in sources(root, ".h"):
        text = open(path, encoding="utf-8", errors="replace").read()

        for unused, name in pattern.findall(text):
            found.add(name)

    return found


def sources(root, suffix):
    found = []

    for directory, unused, names in os.walk(root):
        for name in sorted(names):
            if name.endswith(suffix):
                found.append(os.path.join(directory, name))

    return found


def pointerNames(root):
    found = collections.Counter()

    for path in sources(root, ".h"):
        text = open(path, encoding="utf-8", errors="replace").read()

        for unused, name in MEMBER.findall(text):
            found[name] += 1

    return found


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    pointers = pointerNames("src")
    wide = namesOf("src", WIDE) - set(pointers)
    narrow = namesOf("src", NARROW) - set(pointers)
    floaty = namesOf("src", FLOATY) - set(pointers)
    onPointer = []
    onScalar = []
    reached = 0
    kept = 0
    other = 0

    for path in sources("src", ".cpp"):
        shown = os.path.relpath(path, "src").replace(os.sep, "/")
        text = open(path, encoding="utf-8", errors="replace").read()
        lines = comments.code(text).split("\n")
        # Read a statement at a time, since an expression can continue on the next line.
        at = 1
        pending = ""

        for offset, line in enumerate(lines, 1):
            if not pending:
                at = offset

            pending += " " + line

            if ";" not in pending and "{" not in pending and "}" not in pending:
                continue

            # Close up whitespace around `->` and `.` so a split member access reads whole.
            for expr in PUN.findall(re.sub(r"\s*(->|\.)\s*", r"\1", pending)):
                if any(mark in expr for mark in REACHES):
                    reached += 1
                elif expr in pointers:
                    onPointer.append((shown, at, expr))
                elif expr in narrow or expr in floaty:
                    kept += 1
                # A narrow local or parameter shadows a header member of the same name.
                elif re.search(r"\b(?:short|char|WORD|BYTE)\s+" + re.escape(expr) + r"\b", text):
                    kept += 1
                # A local float is a keep, like a member float.
                elif re.search(r"\b(?:float|double)\s+" + re.escape(expr) + r"\b", text):
                    kept += 1
                # A local pointer is a fault too. Anchored at a line start so
                # `f(nCount * pTotal)` is not read as a declaration.
                elif re.search(r"^[ \t]*(?:const |volatile |struct |class )*\w[\w:]*\s*\*\s*"
                               + re.escape(expr) + r"\s*[;,=]", text, re.M):
                    onPointer.append((shown, at, expr))
                elif expr in wide:
                    onScalar.append((shown, at, expr))
                else:
                    other += 1

            pending = ""

    if show:
        for shown, at, expr in onPointer:
            print("POINTER  %-44s %-5d %s" % (shown, at, expr))
        for shown, at, expr in onScalar:
            print("SCALAR   %-44s %-5d %s" % (shown, at, expr))

    print("%d pun(s) land on a declared pointer and %d on a four-byte integer, both faults; "
          "%d reach a float or a cell through one and %d land on a float or on a field the "
          "cast reaches past the end of, both keeps; %d land on a name no header declares"
          % (len(onPointer), len(onScalar), reached, kept, other))

    return 0


if __name__ == "__main__":
    sys.exit(main())
