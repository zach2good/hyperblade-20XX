"""Print every declaration of a class, so the views of one object can be read side by side.

    python tools/show_class.py AnimNode BroadcastCam

tools/view_classes.py says which classes are declared more than once; this says what each
declaration claims, which is the part that settles whether they are one class.
"""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")


def sourceNames(root):
    # Walk subdirectories; names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def declarations(name):
    pattern = re.compile(r"^class %s(?: : public \w+)?\n\{\n.*?^\};" % re.escape(name),
                         re.S | re.M)
    found = []

    for entry in sorted(sourceNames(SRC)):
        if not entry.endswith((".cpp", ".h")):
            continue

        path = os.path.join(SRC, entry)
        text = open(path, encoding="utf-8", errors="replace").read().replace("\r\n", "\n")

        for match in pattern.finditer(text):
            found.append((entry, match.group(0)))

    return found


def main():
    names = sys.argv[1:]

    if not names:
        raise SystemExit("name at least one class")

    for name in names:
        for entry, body in declarations(name):
            print("=== %s" % entry)
            print(body)
            print("")

    return 0


if __name__ == "__main__":
    sys.exit(main())
