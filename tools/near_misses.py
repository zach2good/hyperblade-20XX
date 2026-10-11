"""Ranks unfinished functions by what closing them is worth, not by how far off they are.

Scoring is all or nothing per function, so the prize for finishing one is its whole size and
the cost is its residue. Ranking by bytes not yet matched buries a large function that is one
instruction away.

Residue is every line in an orig-only or recomp-only diff group, counted on both sides, so one
substituted instruction reads as two. A stub's residue measures the stub, not the work: stubs
(the report's `stub` flag, or 0.00) are excluded from --max-residue unless --stubs is given,
and are marked when listed. Rows in symbols/retired.csv and symbols/blocked_rows.csv are
excluded. A small residue is a candidate, not a promise.

    python tools/near_misses.py                  # the 40 best prizes
    python tools/near_misses.py --max-residue 4  # residue of 4 or less
    python tools/near_misses.py --all            # every unfinished function
    python tools/near_misses.py --limit N        # rows to print
    python tools/near_misses.py --retired        # print the retirement register
"""
import argparse
import csv
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPORT = os.path.join(ROOT, "build", "report.json")
INVENTORY = os.path.join(ROOT, "symbols", "functions.csv")
RETIRED = os.path.join(ROOT, "symbols", "retired.csv")
BLOCKED = os.path.join(ROOT, "symbols", "blocked_rows.csv")
GAME_CODE_LIMIT = 0x004A0000


def sizes():
    out = {}
    with open(INVENTORY, encoding="utf-8") as handle:
        for row in csv.DictReader(line for line in handle if not line.startswith("#")):
            try:
                out[int(row["address"], 16)] = int(row["size"])
            except (KeyError, ValueError):
                continue
    return out


def retired():
    """Addresses whose last bytes no spelling has reached, with the wall that stops each.

    Not missing code. Every one is implemented and behaving; what is retired is the attempt
    to make the remaining instructions agree. Read it as "not feasibly improvable with the
    levers known when the row was written", and re-test a class when a new lever turns up.
    """
    out = {}
    if not os.path.exists(RETIRED):
        return out
    with open(RETIRED, encoding="utf-8") as handle:
        for row in csv.DictReader(line for line in handle if not line.startswith("#")):
            try:
                out[int(row["address"], 16)] = row.get("wall", "")
            except (KeyError, ValueError):
                continue

    # A blocked row is excluded the same way.
    if os.path.exists(BLOCKED):
        with open(BLOCKED, encoding="utf-8") as handle:
            for row in csv.DictReader(line for line in handle if not line.startswith("#")):
                try:
                    out[int(row["address"], 16)] = row.get("reason", "")
                except (KeyError, ValueError):
                    continue

    return out


def residue(entry):
    """Instructions on either side that did not pair."""
    total = 0
    for group in entry.get("diff", []):
        if not isinstance(group, list) or len(group) < 2:
            continue
        for block in group[1]:
            for key in ("orig", "recomp"):
                total += len(block.get(key, []))
    return total


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--max-residue", type=int, default=0,
                        help="only functions whose residue is this or smaller")
    parser.add_argument("--all", action="store_true", help="list every unfinished function")
    parser.add_argument("--stubs", action="store_true",
                        help="include functions still at 0.00, whose residue measures the stub")
    parser.add_argument("--limit", type=int, default=40)
    parser.add_argument("--retired", action="store_true",
                        help="print the retirement register and stop")
    args = parser.parse_args()

    if args.retired:
        dead = retired()
        if not dead:
            print("No retirement register at %s." % RETIRED)
            return 0
        print("%-12s %-22s %s" % ("address", "wall", "name"))
        with open(RETIRED, encoding="utf-8") as handle:
            for row in csv.DictReader(line for line in handle if not line.startswith("#")):
                print("%-12s %-22s %s"
                      % (row.get("address", ""), row.get("wall", ""), row.get("name", "")))
        print()
        print("%d functions retired. Each row needs a measurement behind it;" % len(dead))
        print("check_regressions.py fails if any of them is exact.")
        return 0

    if not os.path.exists(REPORT):
        print("No report. Run:")
        print("  python -m reccmp.tools.asmcmp --target HYPERX --json build/report.json --silent")
        return 1

    size_of = sizes()
    dead = retired()
    rows = []
    for entry in json.load(open(REPORT, encoding="utf-8"))["data"]:
        try:
            address = int(str(entry.get("address", "")), 16)
        except ValueError:
            continue
        if address >= GAME_CODE_LIMIT:
            continue
        share = float(entry.get("matching", 0))
        if share >= 1.0:
            continue
        prize = size_of.get(address, 0)
        if not prize:
            continue
        if address in dead:
            continue
        rows.append((prize, residue(entry), share * 100, address, str(entry.get("name", "")),
                     bool(entry.get("stub", False))))

    if args.max_residue:
        rows = [r for r in rows if r[1] <= args.max_residue
                and (args.stubs or not (r[5] or r[2] <= 0.0))]
    rows.sort(reverse=True)
    shown = rows if args.all else rows[:args.limit]

    print("%-7s %-8s %-7s %-11s %s" % ("prize", "residue", "now", "address", "name"))
    for prize, res, share, address, name, stub in shown:
        tail = ""
        if stub or share <= 0.0:
            tail = "   (stub: residue measures the stub, not the work)"
        print("%-7d %-8d %6.2f  0x%08x  %s%s" % (prize, res, share, address, name, tail))

    print()
    print("%d unfinished game functions listed of %d; %d bytes of prize in the list"
          % (len(shown), len(rows), sum(r[0] for r in shown)))
    if dead and not args.retired:
        print("%d functions excluded: implemented, but no spelling has reached their last"
              " bytes; see symbols/retired.csv" % len(dead))
    tight = [r for r in rows if r[1] <= 4]
    print("%d of them have a residue of 4 instructions or fewer, worth %d bytes"
          % (len(tight), sum(r[0] for r in tight)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
