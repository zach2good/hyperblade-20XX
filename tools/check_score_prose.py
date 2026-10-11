# Report a source comment that records match history instead of describing the code.
#
# Scores, measurement rounds and tool names belong in git history and the docs. A comment gives
# the code's behaviour, or a one-line reason for an unusual spelling stated as a fact about the
# original binary. The patterns below are the forms that history took in this tree: a score with
# a percent sign or a "from", "to exact", "effective match", "round N", worktrees, the proxy,
# the residue and reccmp itself.
#
# Read only.

import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

HISTORY = re.compile(
    r"reccmp"
    r"|effective match"
    r"|\b\d{2,3}\.\d{1,2} ?%"
    r"|\bfrom \d{2,3}\.\d{2}\b"
    r"|\b(?:to|went|goes|is now) exact\b"
    r"|\bround \d+\b"
    r"|\bworktrees?\b"
    r"|\b(?:scores?|scored|measures?|measured) (?:at )?\d{2,3}\.\d{2}\b"
    r"|\bthe proxy\b"
    r"|\bresidue\b",
    re.IGNORECASE)


def main():
    os.chdir(ROOT)
    faults = []

    for suffix in ("*.cpp", "*.h"):
        for path in sorted(glob.glob(os.path.join("src", "**", suffix), recursive=True)):
            shown = path.replace(os.sep, "/")
            lines = open(shown, encoding="utf-8", errors="replace").read().splitlines()

            for n, line in enumerate(lines):
                text = line.lstrip()

                if not text.startswith("//"):
                    continue

                hit = HISTORY.search(text)

                if hit:
                    faults.append((shown, n + 1, hit.group(0)))

    for shown, n, what in faults:
        print("%s:%d records match history (%s)" % (shown, n, what))

    print("%d comment line(s) that record match history" % len(faults))

    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
