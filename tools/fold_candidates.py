"""Rank the functions the original folds bodies onto, where this build does not.

Reads build/report.json and counts every changed line where the original names one function and
this build names another: a call, a pushed or stored function address, or a vtable slot. A body
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
        return orig.split(":  ")[-1], recomp.split(":  ")[-1]

    found_orig = OPERAND.search(orig)
    found_recomp = OPERAND.search(recomp)
    if found_orig is None or found_recomp is None:
        return None

    return found_orig.group(1), found_recomp.group(1)


def main():
    if not os.path.exists(REPORT):
        print("no build/report.json; run python tools/foldcmp.py --target HYPERX --json build/report.json --silent")
        return 1

    counts = collections.Counter()
    ours = collections.defaultdict(set)
    rows = collections.defaultdict(set)
    for row in json.load(open(REPORT))["data"]:
        if row.get("matching", 1) >= 1:
            continue

        for _, blocks in row.get("diff", []):
            for block in blocks:
                for orig, recomp in zip(block.get("orig", []), block.get("recomp", [])):
                    pair = names(row, orig[1], recomp[1])
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
