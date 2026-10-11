"""Split the remaining accuracy by the shape of each row's difference.

It reads build/report.json and buckets each non-exact row by instruction count:

    ours LONGER   this build emits instructions the client has not got. reccmp also pulls the
                  client's padding into the comparison, so the row is penalised twice.
    ours SHORTER  the client emits instructions this build has not got, usually a call this
                  build inlines or a spill it optimises away.
    same length   the difference is registers, operands or scheduling.

Rows listed in symbols/blocked_rows.csv are subtracted, so the totals are what is reachable.

    python tools/gap_shape.py            # the three buckets
    python tools/gap_shape.py --rows     # and the worst rows in each
    python tools/gap_shape.py --why      # what the same-length rows differ by
    python tools/gap_shape.py --frames   # rows whose stack frame is a different size
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import report

BLOCKED = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                       "symbols", "blocked_rows.csv")


def blocked():
    """The rows whose gap is measured and refused, by address."""
    found = {}

    if not os.path.exists(BLOCKED):
        return found

    for line in open(BLOCKED, encoding="utf-8"):
        line = line.strip()

        if not line or line.startswith("#"):
            continue

        parts = line.split(",")

        # Skip the header row.
        if not parts[0].startswith("0x"):
            continue

        found[int(parts[0], 16)] = parts[2]

    return found


def shape(row):
    """(client instructions, our instructions) over every group of one row's diff."""
    orig = 0
    ours = 0

    for _, groups in row["diff"]:
        for group in groups:
            orig += len(group.get("orig", []))
            ours += len(group.get("recomp", []))

    return orig, ours


REGISTER = re.compile(r"\b(?:e?(?:ax|bx|cx|dx|si|di|bp|sp)|[abcd][lh])\b")


def sameLength(rows, refused):
    """The same-length rows split by what differs: register names, operand values, or blocks."""
    kinds = {"register names only": [0.0, 0], "operand values differ": [0.0, 0],
             "blocks moved": [0.0, 0]}

    for row in rows:
        if row.get("effective") or row.get("matching", 1) >= 1 or "diff" not in row:
            continue

        if "vftable" in row["name"] or int(row["address"], 16) in refused:
            continue

        orig, ours = shape(row)

        if orig != ours:
            continue

        names = True
        mnemonics = True

        for _, groups in row["diff"]:
            for group in groups:
                left = [line[1].split(chr(9))[0].strip() for line in group.get("orig", [])]
                right = [line[1].split(chr(9))[0].strip() for line in group.get("recomp", [])]

                for index in range(min(len(left), len(right))):
                    if left[index] == right[index]:
                        continue

                    if REGISTER.sub("R", left[index]) != REGISTER.sub("R", right[index]):
                        names = False

                    if left[index].split()[0] != right[index].split()[0]:
                        mnemonics = False

        key = ("register names only" if names
               else "operand values differ" if mnemonics else "blocks moved")
        kinds[key][0] += (1 - row["matching"]) * 100
        kinds[key][1] += 1

    return [(name, value, count) for name, (value, count) in
            sorted(kinds.items(), key=lambda pair: -pair[1][0])]


FRAME = re.compile(r"^sub esp, (0x[0-9a-f]+|[0-9]+)$")


def frames(rows):
    """The rows whose stack frame is a different size from the client's, and by how much.

    A delta shared by every row would point at a compiler flag; a scattered one is about each
    function's locals.
    """
    found = []

    for row in rows:
        if row.get("effective") or row.get("matching", 1) >= 1 or "diff" not in row:
            continue

        if "vftable" in row["name"]:
            continue

        theirs = None
        ours = None

        for _, groups in row["diff"]:
            for group in groups:
                for line in group.get("orig", []):
                    hit = FRAME.match(line[1].split(chr(9))[0].strip())

                    if hit and theirs is None:
                        theirs = int(hit.group(1), 0)

                for line in group.get("recomp", []):
                    hit = FRAME.match(line[1].split(chr(9))[0].strip())

                    if hit and ours is None:
                        ours = int(hit.group(1), 0)

        if theirs is not None and ours is not None and theirs != ours:
            found.append((ours - theirs, theirs, ours, row["name"]))

    return sorted(found)


def main():
    rows = report.load()
    refused = blocked()
    held = 0.0
    buckets = {"ours LONGER": [], "ours SHORTER": [], "same length": []}

    for row in rows:
        if row.get("effective") or row.get("matching", 1) >= 1 or "diff" not in row:
            continue

        # The headline does not count vtable rows.
        if "vftable" in row["name"]:
            continue

        orig, ours = shape(row)
        gain = (1 - row["matching"]) * 100

        if int(row["address"], 16) in refused:
            held += gain
            continue

        if ours > orig:
            buckets["ours LONGER"].append((gain, ours - orig, row["name"]))
        elif ours < orig:
            buckets["ours SHORTER"].append((gain, orig - ours, row["name"]))
        else:
            buckets["same length"].append((gain, 0, row["name"]))

    total = sum(sum(x[0] for x in v) for v in buckets.values())

    for name in ("ours LONGER", "ours SHORTER", "same length"):
        found = buckets[name]
        points = sum(x[0] for x in found) / 3307.0
        print("%-13s %6.4f points  %3d row(s)  %4.1f%%"
              % (name, points, len(found), 100 * sum(x[0] for x in found) / total))

        if "--rows" in sys.argv[1:]:
            for gain, delta, label in sorted(found, key=lambda x: -x[0])[:8]:
                print("                %6.2f  %+5d  %s" % (gain, delta, label[:48]))

    if "--frames" in sys.argv[1:]:
        found = frames(rows)
        print("%d row(s) whose frame is not the client's size:" % len(found))

        for delta, theirs, ours, label in found:
            print("  %+5d  client 0x%-4x ours 0x%-4x  %s" % (delta, theirs, ours, label[:44]))

        print("")

    if "--why" in sys.argv[1:]:
        print("")
        print("of the same-length rows, by what actually differs:")

        for name, points, count in sameLength(rows, refused):
            print("  %-22s %6.4f points  %3d row(s)" % (name, points / 3307.0, count))

        print("")

    print("%-13s %6.4f points  REACHABLE" % ("TOTAL", total / 3307.0))
    print("%-13s %6.4f points  %3d row(s) measured and refused, see symbols/blocked_rows.csv"
          % ("held back", held / 3307.0, len(refused)))

    return 0


if __name__ == "__main__":
    sys.exit(main())
