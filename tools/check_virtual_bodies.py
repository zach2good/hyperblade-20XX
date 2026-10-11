"""Fail when a class declares a virtual it never defines.

A virtual declared without `= 0` and without an inline body promises a definition. VC6 asks for
one only when it emits the class's vtable, and it emits that only when something constructs the
class, so a class that is never constructed can carry a declaration nothing answers and the
build stays green.

A class that defines nothing anywhere is a dispatch shim, such as `VirtualDtorObject`, and is
passed over: its virtuals are calls, not promises. The fault is a class that defines some of its
members and leaves one virtual undefined.

    python tools/check_virtual_bodies.py
    python tools/check_virtual_bodies.py --all     every virtual it read

Exit code 1 when a declared virtual has no definition and no body.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

CLASS = re.compile(r"^(?:class|struct) ([0-9A-Za-z_]+)(?:\s*:\s*public [0-9A-Za-z_]+)?\s*"
                   + chr(10) + r"\s*\{(.*?)" + chr(10) + r"\};", re.S | re.M)

# A virtual declaration that ends in a semicolon: no `= 0`, no inline `{`.
DECLARED = re.compile(r"^\s+virtual\s+([^;{]*?)\s*;\s*$", re.M)
# A "(*" opens a declarator, as in `float (*GetMatrixPtr())[4]`, not the parameter list.
NAME = re.compile(r"(~?[0-9A-Za-z_]+)\s*\((?!\*)")


def sourceNames(root):
    # Sources live in subdirectories. Names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def sources(suffixes):
    for name in sorted(sourceNames(SRC)):
        if name.endswith(suffixes):
            text = open(os.path.join(SRC, name), encoding="utf-8", errors="replace").read()

            yield name, text.replace(chr(13) + chr(10), chr(10))


def declared():
    """[(class, method, header)] for every virtual that promises a definition."""
    found = []
    seen = set()

    for name, text in sources((".h", ".cpp")):
        for match in CLASS.finditer(text):
            klass = match.group(1)

            if klass in seen:
                continue

            seen.add(klass)

            for declaration in DECLARED.findall(match.group(2)):
                if "= 0" in declaration or declaration.endswith(")") is False:
                    pass

                if "=" in declaration.split("(")[0]:
                    continue

                if declaration.rstrip().endswith("= 0"):
                    continue

                hit = NAME.search(declaration)

                if hit:
                    found.append((klass, hit.group(1), name))

    return found


def defined():
    """Every `Klass::Method` this tree defines, as a set of pairs."""
    found = set()
    pattern = re.compile(r"([0-9A-Za-z_]+)::(~?[0-9A-Za-z_]+)\s*\(")

    for name, text in sources((".cpp", ".h")):
        for match in pattern.finditer(text):
            found.add((match.group(1), match.group(2)))

    return found


def main():
    promises = declared()
    bodies = defined()
    defines = set(klass for klass, unused in bodies)
    missing = [row for row in promises
               if (row[0], row[1]) not in bodies and row[0] in defines]
    shims = sorted(set(row[0] for row in promises
                       if (row[0], row[1]) not in bodies and row[0] not in defines))

    if "--all" in sys.argv[1:]:
        for klass, method, name in promises:
            mark = "     " if (klass, method) in bodies else "  NO "
            print("%s%-28s %s::%s" % (mark, name, klass, method))

    for klass, method, name in missing:
        print("%s  %s::%s is declared virtual and never defined"
              % (name, klass, method))

    print("%d virtual(s) promising a definition, %d dispatch shim(s) passed over, %d without one"
          % (len(promises), len(shims), len(missing)))

    if missing:
        print("Define it, or declare it pure if nothing answers the slot.")

        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
