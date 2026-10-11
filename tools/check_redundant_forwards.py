# Finds a forward declaration in a .cpp for a class an include above it already defines.
#
# Usage: python tools/check_redundant_forwards.py [--list]
#   --list  also print a summary line when there is nothing to report.
# Read only. Exits 1 on any redundant forward.
#
# These are often left behind after a body move. Only includes above
# the forward count: a forward written before the defining include lets an earlier header name
# the type, and the tree does that on purpose. Headers are not read, since a header's forward
# declarations are its own deliberate list.

import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

FORWARD = re.compile(r"^\s*(?:class|struct)\s+(\w+)\s*;\s*$")
DEFINES = re.compile(r"^\s*(?:class|struct)\s+(\w+)\s*(?::[^;{]*)?\{", re.M)
INCLUDE = re.compile(r'^#include\s+"([^"]+)"')

SKIP = ("src/jpeg/",)


def headers():
    """Each header's class definitions and the headers it pulls in, keyed without src/."""
    defines = {}
    pulls = {}

    for path in sorted(glob.glob(os.path.join("src", "**", "*.h"), recursive=True)):
        shown = path.replace(os.sep, "/")

        if shown.startswith(SKIP):
            continue

        key = shown[len("src/"):]
        body = code(open(shown, encoding="utf-8", errors="replace").read())
        defines[key] = set(DEFINES.findall(body))
        pulls[key] = []

        for line in body.split("\n"):
            found = INCLUDE.match(line)

            if found:
                pulls[key].append(found.group(1).replace("\\", "/"))

    for key in pulls:
        here = os.path.dirname(key)
        pulls[key] = [inc if inc in defines else
                      (here + "/" + inc if here and (here + "/" + inc) in defines else inc)
                      for inc in pulls[key]]

    return defines, pulls


def reach(key, defines, pulls, seen):
    if key in seen or key not in defines:
        return set()

    seen.add(key)
    out = set(defines[key])

    for inc in pulls.get(key, []):
        out |= reach(inc, defines, pulls, seen)

    return out


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    defines, pulls = headers()
    faults = []
    forwards = 0

    for path in sorted(glob.glob(os.path.join("src", "**", "*.cpp"), recursive=True)):
        shown = path.replace(os.sep, "/")

        if shown.startswith(SKIP):
            continue

        raw = open(shown, encoding="utf-8", errors="replace").read()
        here = os.path.dirname(shown[len("src/"):])
        visible = set()
        seen = set()

        for n, line in enumerate(code(raw).split("\n"), 1):
            found = INCLUDE.match(line)

            if found:
                inc = found.group(1).replace("\\", "/")
                key = inc if inc in defines else (here + "/" + inc if here else inc)
                visible |= reach(key, defines, pulls, seen)
                continue

            what = FORWARD.match(line)

            if not what:
                continue

            forwards += 1

            if what.group(1) in visible:
                faults.append((shown, n, what.group(1)))

    for shown, n, name in faults:
        print("%-44s %s is defined by an include above this line"
              % ("%s:%d" % (shown, n), name))

    if show and not faults:
        print("%d forward declaration(s) in .cpp files, and every one names a class no include "
              "above it defines" % forwards)

    print("%d forward declaration(s) in .cpp files, %d for a class an include above already "
          "defines" % (forwards, len(faults)))

    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
