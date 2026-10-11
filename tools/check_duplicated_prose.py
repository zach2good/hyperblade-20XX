# Find a run of comment lines written twice inside one note.
#
#     python tools/check_duplicated_prose.py           # exit 1 on any repeated run
#     python tools/check_duplicated_prose.py --list    # also confirm when there are none
#
# A reader who reaches the second copy cannot tell a paste from a second measurement. The unit is
# a run of two or more consecutive lines: a single repeated line can be a deliberate restatement
# of the same fact, so it is not gated.
#
# Read only.

import glob
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SKIP = ("src/jpeg/",)

# Two consecutive lines, each long enough that a repeat is not a coincidence such as a listing's
# `//   mov eax, ebx` twice over.
RUN = 2
ENOUGH = 20


def scan(shown, run, start, hits):
    text = [line[2:].strip() for line in run]
    n = len(text)

    for i in range(n):
        for j in range(i + RUN, n):
            k = 0

            while j + k < n and i + k < j and text[i + k] == text[j + k]:
                k += 1

            if k >= RUN and all(len(text[i + m]) >= ENOUGH for m in range(k)):
                hits.append((shown, start + i + 1, start + j + 1, k, text[i]))

                return


def notes(shown):
    """Each contiguous run of comment lines in one file, with the line it starts on."""
    lines = open(shown, encoding="utf-8", errors="replace").read().splitlines()
    run = []
    start = 0

    for n, line in enumerate(lines):
        if line.lstrip().startswith("//"):
            if not run:
                start = n

            run.append(line.lstrip())
            continue

        if len(run) >= 2 * RUN:
            yield run, start

        run = []

    if len(run) >= 2 * RUN:
        yield run, start


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

            for run, start in notes(shown):
                scan(shown, run, start, hits)

    for shown, first, again, k, line in hits:
        print("%-36s lines %d and %d, %d line(s): %s"
              % (shown, first, again, k, line[:56]))

    if show and not hits:
        print("no note in %d file(s) writes a run of lines twice" % read)

    print("%d note(s) with a run of %d or more comment lines written twice" % (len(hits), RUN))

    return 1 if hits else 0


if __name__ == "__main__":
    sys.exit(main())
