"""Rank the functions the original folds bodies onto, where this build does not.

Reads build/report.json and counts every changed line where the original names one function and
this build names another: a call, a pushed or stored function address, or a vtable slot, paired
by its offset. An original `__purecall` against a body here shows as one more target. A body
the original folds onto a marked function shows up once per site. MATCHING.md, Link folding, says
how to put a body in the original's group.

    python tools/fold_candidates.py            # the top twenty
    python tools/fold_candidates.py --all
"""

import collections
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPORT = os.path.join(ROOT, "build", "report.json")

OPERAND = re.compile(r"(?:call|push|mov [^,]+,)\s+(.+?) \(FUNCTION\)")
VTABLE = 5


def names(row, orig, recomp):
    """Answers the two function names a changed line holds, or None."""
    if row.get("type") == VTABLE:
        # A slot with no name on either side, such as an unannotated runtime table's, says nothing.
        if ":  " not in orig or ":  " not in recomp:
            return None

        return orig.split(":  ")[-1], recomp.split(":  ")[-1]

    found_orig = OPERAND.search(orig)
    found_recomp = OPERAND.search(recomp)
    if found_orig is None or found_recomp is None:
        return None

    return found_orig.group(1), found_recomp.group(1)


def changed_lines(row):
    """Answers the (original, ours) text of each changed line.

    A vtable's diff can put a slot's two sides in separate blocks, so its lines pair by slot offset.
    """
    if row.get("type") != VTABLE:
        for _, blocks in row.get("diff", []):
            for block in blocks:
                for orig, recomp in zip(block.get("orig", []), block.get("recomp", [])):
                    yield orig[1], recomp[1]

        return

    sides = {"orig": {}, "recomp": {}}
    for _, blocks in row.get("diff", []):
        for block in blocks:
            for side, slots in sides.items():
                for offset, text in block.get(side, []):
                    slots[offset] = text

    for offset, orig in sides["orig"].items():
        if offset in sides["recomp"]:
            yield orig, sides["recomp"][offset]


def main():
    if not os.path.exists(REPORT):
        print("no build/report.json; run python tools/foldcmp.py --target HYPERX1001 --json build/report.json --silent")
        return 1

    counts = collections.Counter()
    ours = collections.defaultdict(set)
    rows = collections.defaultdict(set)
    for row in json.load(open(REPORT))["data"]:
        if row.get("matching", 1) >= 1:
            continue

        for orig, recomp in changed_lines(row):
            pair = names(row, orig, recomp)
            if pair is None or pair[0] == pair[1]:
                continue

            counts[pair[0]] += 1
            ours[pair[0]].add(pair[1])
            rows[pair[0]].add(row["name"])

    limit = None if "--all" in sys.argv else 20
    for target, count in counts.most_common(limit):
        print("%3d  %-50s <- %s" % (count, target, ", ".join(sorted(ours[target])[:2])))
        print("     in %s" % ", ".join(sorted(rows[target])[:3]))

    print("%d target(s) over %d site(s)" % (len(counts), sum(counts.values())))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
