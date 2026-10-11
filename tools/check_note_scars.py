# Find a bare `//` that opens a note, closes one, or stands next to another.
#
#     python tools/check_note_scars.py           # exit 1 on any
#     python tools/check_note_scars.py --list    # also confirm when there are none
#
# A bare `//` between two paragraphs separates them and is left alone. One at the top or bottom
# of a run, or two in a row, separates nothing; it is usually left when a note is cut and pasted.
#
# It gates because several checkers treat a comment line above a body as its explanation, and a
# bare `//` would pass as one.
#
# Read only.

import glob
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SKIP = ("src/jpeg/",)


def bare(line):
    return line.strip() == "//"


def scars(lines):
    """The index of every bare // that opens a run, closes one, or doubles another."""
    found = set()
    run = []
    start = 0

    def look(run, start):
        if not run:
            return

        n = len(run)
        head = 0

        while head < n and bare(run[head]):
            found.add(start + head)
            head += 1

        tail = n - 1

        while tail > head and bare(run[tail]):
            found.add(start + tail)
            tail -= 1

        for i in range(head, tail):
            if bare(run[i]) and bare(run[i + 1]):
                found.add(start + i + 1)

    for n, line in enumerate(lines):
        if line.lstrip().startswith("//"):
            if not run:
                start = n

            run.append(line)
            continue

        look(run, start)
        run = []

    look(run, start)

    return found


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    faults = []
    read = 0

    for suffix in ("*.cpp", "*.h"):
        for path in sorted(glob.glob(os.path.join("src", "**", suffix), recursive=True)):
            shown = path.replace(os.sep, "/")

            if shown.startswith(SKIP):
                continue

            read += 1
            lines = open(shown, encoding="utf-8", errors="replace").read().splitlines()

            for n in sorted(scars(lines)):
                faults.append((shown, n + 1))

    for shown, n in faults:
        print("%s:%d is a bare // that opens, closes or doubles a note" % (shown, n))

    if show and not faults:
        print("no note in %d file(s) opens, closes or doubles on a bare //" % read)

    print("%d bare // line(s) that separate nothing" % len(faults))

    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
