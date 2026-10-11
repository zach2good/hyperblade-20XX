# Report a `.cpp` extern that restates one a header it includes already declares, with no reason.
#
# The tree sometimes restates a global on purpose, with a note beside the body that reads it.
# This fails only on a restatement with no comment above it. A restated type can also drift: C++
# mangles `volatile` into the name, so two units that disagree do not link to the same object.
#
# Includes are followed transitively, relative to the including file. src/globals.cpp is the
# definition file, where an `extern` above each definition binds the GLOBAL marker, and is skipped.
#
#   python tools/check_restated_externs.py
#   python tools/check_restated_externs.py --list    # also confirm when every one has a reason
#
# Read only. Exit code 1 when a restatement has no reason above it.

import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

EXTERN = re.compile(r"^extern\s+(?!\"C\")([^;=]*?\b(\w+))\s*(?:\[[^\]]*\])?\s*;\s*$", re.M)
INCLUDE = re.compile(r'^#include\s+"([^"]+)"', re.M)

SKIP = ("src/jpeg/",)
DEFINITIONS = ("src/globals.cpp",)


def headers():
    """Each header's extern names and the headers it pulls in, keyed without the src/ prefix."""
    declares = {}
    pulls = {}

    for path in sorted(glob.glob(os.path.join("src", "**", "*.h"), recursive=True)):
        shown = path.replace(os.sep, "/")

        if shown.startswith(SKIP):
            continue

        key = shown[len("src/"):]
        body = code(open(shown, encoding="utf-8", errors="replace").read())
        declares[key] = set(name for unused, name in EXTERN.findall(body))
        pulls[key] = [inc.replace("\\", "/") for inc in INCLUDE.findall(body)]

    # An include is relative to the file that writes it: `#include "planstate.h"` inside
    # src/planvm/ is planvm/planstate.h.
    for key in pulls:
        here = os.path.dirname(key)
        pulls[key] = [inc if inc in declares else
                      (here + "/" + inc if here and (here + "/" + inc) in declares else inc)
                      for inc in pulls[key]]

    return declares, pulls


def reach(key, declares, pulls, seen):
    if key in seen or key not in declares:
        return set()

    seen.add(key)
    out = set(declares[key])

    for inc in pulls.get(key, []):
        out |= reach(inc, declares, pulls, seen)

    return out


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    declares, pulls = headers()
    faults = []
    restated = 0

    for path in sorted(glob.glob(os.path.join("src", "**", "*.cpp"), recursive=True)):
        shown = path.replace(os.sep, "/")

        if shown.startswith(SKIP):
            continue

        # Open the file before skipping it, so tools/check_battery_reach.py sees the full reach.
        raw = open(shown, encoding="utf-8", errors="replace").read()

        if shown in DEFINITIONS:
            continue

        lines = raw.splitlines()
        here = os.path.dirname(shown[len("src/"):])
        visible = set()
        seen = set()

        for inc in INCLUDE.findall(code(raw)):
            inc = inc.replace("\\", "/")
            key = inc if inc in declares else (here + "/" + inc if here else inc)
            visible |= reach(key, declares, pulls, seen)

        for n, line in enumerate(lines):
            found = EXTERN.match(line)

            if not found or found.group(2) not in visible:
                continue

            restated += 1

            # A run of restatements shares one comment.
            at = n

            while at > 0 and (EXTERN.match(lines[at - 1]) or lines[at - 1].strip() == ""):
                at -= 1

            if at > 0 and lines[at - 1].lstrip().startswith("//"):
                continue

            faults.append((shown, n + 1, found.group(2)))

    for shown, n, name in faults:
        print("%-44s %s is declared by a header this file includes, with no reason above it"
              % ("%s:%d" % (shown, n), name))

    if show and not faults:
        print("%d extern(s) restate a name an included header declares, and every one says why"
              % restated)

    print("%d restated extern(s), %d with no reason written above them"
          % (restated, len(faults)))

    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
