# Report a comment naming one file on both sides of an "and".
#
# When two files merge into one, a rename turns "a.cpp and b.cpp" into "a.cpp and a.cpp". Fix it
# by naming the bodies the sentence meant. The fault is two names joined by "and", "or" or a
# comma; one file mentioned twice elsewhere in a sentence is fine.
#
#   python tools/check_doubled_file_names.py
#   python tools/check_doubled_file_names.py --list    # also confirm when clean
#
# Read only. Exit code 1 when any is found.

import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SKIP = ("src/jpeg/",)

# <name> [up to three words] and|or|, <the same name>. Either side may carry the src/ path.
PAIR = re.compile(r"\b((?:src/[A-Za-z0-9_/]+/)?([A-Za-z0-9_]+\.(?:cpp|h)))"
                  r"(?:'s)?((?:\s+\w+){0,3}\s+(?:and|or|,)\s+)"
                  r"((?:src/[A-Za-z0-9_/]+/)?\2)\b")


def notes(shown):
    """Each contiguous run of comment lines, joined, with the line it starts on."""
    lines = open(shown, encoding="utf-8", errors="replace").read().splitlines()
    run = []
    start = 0

    for n, line in enumerate(lines):
        if line.lstrip().startswith("//"):
            if not run:
                start = n

            run.append(line.lstrip()[2:].strip())
            continue

        if run:
            yield " ".join(run), start

        run = []

    if run:
        yield " ".join(run), start


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    hits = []
    read = 0

    for suffix in ("*.cpp", "*.h"):
        for path in sorted(glob.glob(os.path.join("src", "**", suffix), recursive=True)):
            shown = path.replace(os.sep, "/")

            if shown.startswith(SKIP):
                continue

            read += 1

            for text, start in notes(shown):
                for found in PAIR.finditer(text):
                    hits.append((shown, start + 1, found.group(0)))

    for shown, n, what in hits:
        print("%-42s %s" % ("%s:%d" % (shown, n), what[:86]))

    if show and not hits:
        print("no note in %d file(s) names one file on both sides of an and" % read)

    print("%d place(s) naming one file twice on either side of an and" % len(hits))

    return 1 if hits else 0


if __name__ == "__main__":
    sys.exit(main())
