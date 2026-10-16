"""Name the function a bring-up crash address falls in.

A bring-up run that faults writes its address into bringup.log. This turns the address into a
name by reading the linker map the bring-up build emits, which lists every function by its
offset from the image base.

    python tools/bringup_where.py 0x0004abcd        # an rva, as the log prints it
    python tools/bringup_where.py 0x0044abcd        # or the address itself
    python tools/bringup_where.py                   # every CRASH line in the last run

The map is only written by a bring-up build, so run tools/bringup_run.py first.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

MAP = os.path.join(ROOT, "build-harness", "out", "hyperx.map")
LOG = os.path.join(ROOT, "build-harness", "run", "bringup.log")

IMAGE_BASE = 0x400000

# 0001:0000abcd  ?Foo@@YAXXZ  0040abcd f  file.obj
ENTRY = re.compile(r"^\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+([0-9a-fA-F]{8})\s")


def symbols():
    if not os.path.exists(MAP):
        raise SystemExit("no %s; run tools/bringup_run.py first" % os.path.relpath(MAP, ROOT))

    found = []

    for line in open(MAP, encoding="utf-8", errors="replace"):
        hit = ENTRY.match(line)

        if hit is None:
            continue

        found.append((int(hit.group(2), 16), hit.group(1)))

    found.sort()

    return found


def report(table, address):
    if address < IMAGE_BASE:
        address += IMAGE_BASE

    best = None

    for start, name in table:
        if start > address:
            break

        best = (start, name)

    if best is None:
        print("0x%08x  before the first symbol" % address)
        return

    print("0x%08x  %s + 0x%x" % (address, best[1], address - best[0]))


def crashLines():
    if not os.path.exists(LOG):
        raise SystemExit("no %s" % os.path.relpath(LOG, ROOT))

    found = []

    for line in open(LOG, encoding="utf-8", errors="replace"):
        hit = re.search(r"CRASH (?:code = \S+ at|from) = (0x[0-9a-fA-F]+)", line)

        if hit is not None:
            found.append(int(hit.group(1), 16))

    return found


def main():
    table = symbols()

    addresses = [int(argument, 16) for argument in sys.argv[1:]]

    if not addresses:
        addresses = crashLines()

        if not addresses:
            print("no CRASH line in the last run")
            return 1

    for address in addresses:
        report(table, address)

    return 0


if __name__ == "__main__":
    sys.exit(main())
