"""Fail when a polymorphic class declares a field at an offset the vptr already holds.

VC6 puts a polymorphic class's four byte vptr at offset zero. A field declared over those bytes
counts them twice, and every later field lands four bytes late.

The rule applies to the first field a class declares, read from its offset comment. In a class
that declares a virtual or derives from one that does, it must be 0x04 or more. A derived
class's first field must also sit above the deepest offset its base chain declares.

    python tools/check_vptr_offsets.py
    python tools/check_vptr_offsets.py --all     every class it read and where it starts

Limit: the base's deepest declared offset is a weaker bound than its true size, so a base
shorter than the derived class assumes goes unseen. Compile-time size assertions catch that.

Exit code 1 when a class's first field overlaps its own vptr.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

CLASS = re.compile(r"^(?:class|struct) ([0-9A-Za-z_]+)(?: *: *public ([0-9A-Za-z_]+))? *"
                   + chr(10) + r"\s*\{(.*?)" + chr(10) + r"\};", re.S | re.M)

# A field and its offset comment.
FIELD = re.compile(r"^\s+(?!virtual|typedef)[\w:*&<>\s]+?[\s*]([0-9A-Za-z_]+)(?:\[[^\]]*\])?;"
                   + r"\s*//\s*(0x[0-9a-fA-F]+)", re.M)

VIRTUAL = re.compile(r"^\s+virtual\b", re.M)


def sourceNames(root):
    # Sources live in subdirectories, so this walks. Names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def sources():
    for name in sorted(sourceNames(SRC)):
        if name.endswith((".cpp", ".h")):
            text = open(os.path.join(SRC, name), encoding="utf-8", errors="replace").read()

            yield name, text.replace(chr(13) + chr(10), chr(10))


def classes():
    found = {}

    for name, text in sources():
        for match in CLASS.finditer(text):
            if match.group(1) in found:
                continue

            fields = FIELD.findall(match.group(3))
            offsets = [int(offset, 16) for unused, offset in fields]

            found[match.group(1)] = (name, match.group(2),
                                     VIRTUAL.search(match.group(3)) is not None,
                                     offsets[0] if offsets else None,
                                     fields[0][0] if fields else None,
                                     max(offsets) if offsets else None)

    return found


def baseFloor(klass, known):
    """The deepest offset anything above this class declares, or None when nothing does."""
    seen = set()
    deepest = None
    klass = known[klass][1]

    while klass in known and klass not in seen:
        seen.add(klass)

        if known[klass][5] is not None:
            deepest = known[klass][5] if deepest is None else max(deepest, known[klass][5])

        klass = known[klass][1]

    return deepest


def polymorphic(klass, known):
    """True when the class or anything above it declares a virtual."""
    seen = set()

    while klass in known and klass not in seen:
        seen.add(klass)

        if known[klass][2]:
            return True

        klass = known[klass][1]

    return False


def main():
    known = classes()
    wrong = []
    checked = 0

    for klass in sorted(known):
        name, base, unused, first, field, unusedToo = known[klass]

        if first is None:
            continue

        if base is not None:
            floor = baseFloor(klass, known)

            if floor is None:
                continue

            checked += 1

            if first <= floor:
                wrong.append((name, klass, field, first, floor + 4))

            if "--all" in sys.argv[1:]:
                print("%-30s %-26s first %s at 0x%03x, base reaches 0x%03x"
                      % (name, klass, field, first, floor))

            continue

        checked += 1
        needed = 4 if polymorphic(klass, known) else 0

        if first < needed:
            wrong.append((name, klass, field, first, needed))

        if "--all" in sys.argv[1:]:
            print("%-30s %-26s first %s at 0x%03x, needs 0x%03x"
                  % (name, klass, field, first, needed))

    for name, klass, field, first, needed in wrong:
        print("%s  %s declares %s at 0x%03x, which is bytes something above it already holds"
              % (name, klass, field, first))
        print("        every field after it lands at least 0x%x late" % (needed - first))

    print("%d class(es) with a declared first field, %d starting inside what is above them"
          % (checked, len(wrong)))

    if wrong:
        print("Shorten the reserve, or drop it and let the base carry those bytes.")

        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
