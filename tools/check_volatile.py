# Report a `volatile` in the source with no reason written beside it.
#
# The original's source had no reason to force a load or a store, so a `volatile` spelled to
# steer the code VC6 emits misstates the program. The qualifier stays only where a value is
# really shared with another thread, the hardware or an interrupt, and the line itself or the
# comment line above it says so: it names "shared", "thread", "hardware" or "interrupt".
#
# Read only.

import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

VOLATILE = re.compile(r"\bvolatile\b")
REASON = re.compile(r"\b(?:shared|threads?|hardware|interrupts?)\b", re.IGNORECASE)


def comment_of(line):
    at = line.find("//")

    if at < 0:
        return ""

    return line[at:]


def main():
    os.chdir(ROOT)
    uses = 0
    faults = []

    for suffix in ("*.cpp", "*.h", "*.c"):
        for path in sorted(glob.glob(os.path.join("src", "**", suffix), recursive=True)):
            shown = path.replace(os.sep, "/")
            lines = open(shown, encoding="utf-8", errors="replace").read().splitlines()

            for n, line in enumerate(lines):
                code = line.split("//", 1)[0]

                if not VOLATILE.search(code):
                    continue

                uses += 1
                above = ""

                if n > 0 and lines[n - 1].lstrip().startswith("//"):
                    above = lines[n - 1]

                if REASON.search(comment_of(line)) or REASON.search(above):
                    continue

                faults.append((shown, n + 1, line.strip()))

    for shown, n, text in faults:
        print("%s:%d volatile with no shared or hardware reason: %s" % (shown, n, text))

    print("%d volatile use(s), %d with no shared or hardware reason" % (uses, len(faults)))

    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
