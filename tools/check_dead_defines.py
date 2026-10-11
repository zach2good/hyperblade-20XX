# Finds a numeric #define in a .cpp that nothing in that file reads.
#
# Usage: python tools/check_dead_defines.py [--list]
#   --list  print each dead define.
# Always exits 0; the count is the result.
#
# A dead define misleads a reader who greps an offset, and tools/check_duplicate_constants.py
# counts it as agreement between files. Comment lines are not uses. Headers are covered by
# tools/dead_defines.py, which searches every file that includes the header.

import os
import re
import sys

DEFINE = re.compile(r"^#define ([A-Z][A-Z0-9_]*) +(?:0x[0-9a-fA-F]+|\d+)\s*$", re.M)


def sourceNames(root):
    # Every file under root, recursively, as a path relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def scan(path):
    text = open(path, encoding="utf-8", errors="replace").read()
    code = "\n".join(line for line in text.split("\n") if not line.lstrip().startswith("//"))

    dead = []

    for name in DEFINE.findall(text):
        if len(re.findall(r"\b%s\b" % name, code)) <= 1:
            dead.append(name)

    return dead


def main():
    root = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "src")
    show = "--list" in sys.argv

    total = 0
    files = 0

    for name in sorted(sourceNames(root)):
        if not name.endswith(".cpp"):
            continue

        dead = scan(os.path.join(root, name))

        if not dead:
            continue

        total += len(dead)
        files += 1

        if show:
            for macro in dead:
                print("src/%s: %s is defined and never read" % (name, macro))

    print("%d define(s) in %d file(s) that nothing reads" % (total, files))

    return 0


if __name__ == "__main__":
    sys.exit(main())
