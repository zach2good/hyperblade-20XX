"""Fail on a statement that assigns something to itself.

A bulk rename can turn a store into one, for example when a member and a parameter share a name,
and the compiler accepts it. The store then silently disappears.

A `volatile` cast on the left is deliberate: it forces VC6 to emit the store where the client
emits one. src/graphics/modeldatabase.cpp has the only such round trip.

    python tools/check_self_assign.py

Exit code 1 when one is found.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

# `x = x;` and `*(T*)&x = x;`, with the cast on the left ignored unless it says volatile.
ASSIGN = re.compile(r"^\s*(?:\*\(([^)]*)\)\s*&)?([0-9A-Za-z_]+(?:(?:\.|->)[0-9A-Za-z_]+)*"
                    + r"(?:\[[0-9]+\])?)\s*=\s*"
                    + r"([0-9A-Za-z_]+(?:(?:\.|->)[0-9A-Za-z_]+)*(?:\[[0-9]+\])?)\s*;\s*$")


def sourceNames(root):
    # Paths come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def main():
    found = []

    for name in sorted(sourceNames(SRC)):
        if not name.endswith((".cpp", ".h")):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8",
                       errors="replace").read().replace(chr(13) + chr(10), chr(10))

        for index, line in enumerate(text.split(chr(10))):
            match = ASSIGN.match(line)

            if match is None or match.group(2) != match.group(3):
                continue

            if match.group(1) and "volatile" in match.group(1):
                continue

            found.append((name, index + 1, line.strip()))

    for name, line, text in found:
        print("%s:%d  %s" % (name, line, text))

    print("%d self-assignment(s), volatile round trips aside" % len(found))

    if found:
        print("Each is a rename that collapsed two names into one. Name one of them apart.")

        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
