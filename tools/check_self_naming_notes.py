# Find comments that name the file they are written in.
#
#     python tools/check_self_naming_notes.py           # count against BUDGET
#     python tools/check_self_naming_notes.py --list    # also print each fault
#
# A note that names its own file is usually left behind when two files merge: a path that once
# pointed at another file now points at itself, and some such notes invert their own claim.
# Repair it by writing "this file" or the function's name, or by recovering what it used to
# point at.
#
# Both the full "src/planvm/planstate.cpp" and the bare "planstate.cpp" count. A partial path
# such as "planvm/damagegate.h" is missed. A line with two self-references is one fault.
#
# Read only. Exits 1 when the count differs from BUDGET.

import collections
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

PATH = re.compile(r"src/[A-Za-z0-9_/]+\.(?:cpp|h)")
BARE = re.compile(r"(?<![/\w.])([A-Za-z0-9_]+\.(?:cpp|h))(?![\w/])")

SKIP = ("jpeg/",)

# A ratchet, not a tolerance: a count above or below it fails.
BUDGET = 0


def comments(text):
    for number, line in enumerate(text.splitlines(), 1):
        at = line.find("//")

        if at >= 0:
            yield number, line[at:], line.strip()


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    faults = collections.defaultdict(list)

    for suffix in ("*.h", "*.cpp"):
        for path in sorted(glob.glob(os.path.join(ROOT, "src", "**", suffix), recursive=True)):
            shown = os.path.relpath(path, os.path.join(ROOT, "src")).replace(os.sep, "/")

            if shown.startswith(SKIP):
                continue

            named = "src/" + shown
            base = os.path.basename(shown)
            text = open(path, encoding="utf-8", errors="replace").read()

            if base not in text:
                continue

            for number, body, whole in comments(text):
                if (any(found.group(0) == named for found in PATH.finditer(body))
                        or any(found.group(1) == base for found in BARE.finditer(body))):
                    faults[named].append((number, whole))

    total = sum(len(where) for where in faults.values())

    if show:
        for named in sorted(faults):
            for number, whole in faults[named]:
                print("%s:%d  %s" % (named, number, whole[:100]))

    print("%d comment(s) naming the file they are written in, over %d file(s), budget %d"
          % (total, len(faults), BUDGET))

    if total > BUDGET:
        print("ABOVE BUDGET by %d. A merge folded two files and left a note pointing at itself; "
              "say what the sentence meant instead of the path." % (total - BUDGET))
        return 1

    if total < BUDGET:
        print("BELOW BUDGET by %d. Lower BUDGET to %d so the figure stays true."
              % (BUDGET - total, total))
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
