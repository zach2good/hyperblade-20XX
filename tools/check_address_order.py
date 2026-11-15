"""Fail when a body breaks its file's address order.

Each .cpp lists its bodies in the order the original placed them. Two rules:

1. HYPERX markers ascend down the file. symbols/address_order.csv lists the descents that predate
   this check, so only a new one fails.
2. A body with a HYPERX100 marker and no HYPERX one, outside v1.0.0.0-only code, sits between the
   nearest HYPERX100 markers above and below it when those two ascend, or on the right side of
   the only one it has. v1.0.0.0's order is not checked otherwise: its link order differs.

    python tools/check_address_order.py
    python tools/check_address_order.py --save    # write the register from today's descents
"""

import csv
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REGISTER = os.path.join(ROOT, "symbols", "address_order.csv")
REASON = "older than this check; moving the body is unmeasured"

# A condition, spaces removed, that holds only in v1.0.0.0, and one that holds only outside it.
V100 = ("HYPERX_V100", "defined(HYPERX_V100)")
NOT_V100 = ("!defined(HYPERX_V100)", "!HYPERX_V100")

MARKER = re.compile(r"^\s*//\s*FUNCTION:\s*(HYPERX100|HYPERX)\s+0x([0-9a-fA-F]+)")


def bodies(path):
    """Answers (line, HYPERX address or None, HYPERX100 address or None) for each marked body."""
    found = []
    pending = {}
    # One [only v1.0.0.0, a branch excluded v1.0.0.0] pair per open #if. Only a branch whose whole
    # condition is HYPERX_V100 counts as v1.0.0.0's; any other shape counts as both.
    stack = []
    for number, line in enumerate(open(path, encoding="utf-8", errors="replace"), 1):
        words = line.split()
        directive = words[0] if words else ""
        condition = "".join(words[1:])
        if directive in ("#if", "#ifdef", "#ifndef"):
            only = condition in V100 and directive != "#ifndef"
            excludes = (directive == "#ifndef" and condition == "HYPERX_V100") or condition in NOT_V100
            stack.append([only, excludes])
            continue

        # After a branch that excludes v1.0.0.0, every later branch is v1.0.0.0's.
        if directive == "#elif" and stack:
            excluded = stack[-1][1]
            stack[-1] = [excluded or condition in V100, excluded]
            continue

        if directive == "#else" and stack:
            stack[-1] = [stack[-1][1], False]
            continue

        if directive == "#endif" and stack:
            stack.pop()
            continue

        hit = MARKER.match(line)
        if hit:
            pending[hit.group(1)] = int(hit.group(2), 16)
            continue

        stripped = line.strip()
        if stripped.startswith("//") or stripped == "":
            continue

        if pending and not any(only for only, _ in stack):
            found.append((number, pending.get("HYPERX"), pending.get("HYPERX100")))

        pending = {}

    return found


def faults(path):
    shown = os.path.relpath(path, ROOT).replace(os.sep, "/")
    found = bodies(path)
    out = []

    last = None
    for number, addr, _ in found:
        if addr is None:
            continue

        if last is not None and addr < last:
            out.append(("descent", shown, addr, number))

        last = addr

    for index, (number, addr, addr100) in enumerate(found):
        if addr is not None or addr100 is None:
            continue

        above = [a for _, _, a in found[:index] if a is not None]
        below = [a for _, _, a in found[index + 1:] if a is not None]
        low = above[-1] if above else None
        high = below[0] if below else None
        if low is not None and high is not None and low > high:
            continue

        if (low is not None and addr100 < low) or (high is not None and addr100 > high):
            out.append(("v100", shown, addr100, number))

    return out


def main():
    os.chdir(ROOT)
    found = []
    for path in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True)):
        found.extend(faults(path))

    descents = sorted((shown, "0x%08x" % addr) for kind, shown, addr, _ in found if kind == "descent")

    reasons = {}
    if os.path.exists(REGISTER):
        rows = (line for line in open(REGISTER, encoding="utf-8") if not line.startswith("#"))
        reasons = {(row["path"], row["address"]): row.get("reason") or REASON for row in csv.DictReader(rows)}

    known = set(reasons)

    if "--save" in sys.argv:
        with open(REGISTER, "w", newline="\n", encoding="utf-8") as handle:
            handle.write("# HYPERX markers that sit below a higher one in their file, from before\n")
            handle.write("# tools/check_address_order.py. A new descent fails; remove a row once it is fixed.\n")
            writer = csv.writer(handle, lineterminator="\n")
            writer.writerow(("path", "address", "reason"))
            for key in descents:
                writer.writerow(key + (reasons.get(key, REASON),))

        print("saved %d descent(s)" % len(descents))
        return 0

    bad = 0
    for kind, shown, addr, number in found:
        if kind == "descent" and (shown, "0x%08x" % addr) in known:
            continue

        bad += 1
        print("%s:%d: %s 0x%08x out of order" % (shown, number, "HYPERX100" if kind == "v100" else "HYPERX", addr))

    stale = known - set(descents)
    for shown, addr in sorted(stale):
        bad += 1
        print("%s: registered descent %s is gone; remove its row" % (shown, addr))

    print("%d body(ies) out of address order, %d registered descent(s)" % (bad, len(known)))
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())
