"""Finds functions whose diff is missing an object load the client has.

Where the client materialises an object into ECX and this build does not, the diff shows an
orig-only group that is nothing but `mov ecx, <reg>`. That is the signature of a callee declared
as a free function where the original has a member. A `__thiscall` member with one stack
argument that never touches `this` compiles to the same body as a `__stdcall` free function,
so Ghidra cannot tell the two apart.

This is a candidate list. Some hits are the one-instruction-early scheduling wall instead. Read
the diff before converting anything.

    python tools/missing_this.py
    python tools/missing_this.py --max-residue 6
"""
import argparse
import csv
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPORT = os.path.join(ROOT, "build", "report.json")
INVENTORY = os.path.join(ROOT, "symbols", "functions.csv")
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


def scan(entry):
    """(orig-only groups that are purely object loads, total residue)."""
    hits = 0
    residue = 0
    for group in entry.get("diff", []):
        if not isinstance(group, list) or len(group) < 2:
            continue
        for block in group[1]:
            orig = block.get("orig", [])
            recomp = block.get("recomp", [])
            residue += len(orig) + len(recomp)
            if not orig or recomp:
                continue
            lines = [str(row[1]) if isinstance(row, list) and len(row) > 1 else str(row)
                     for row in orig]
            if all(line.strip().startswith("mov ecx,") for line in lines):
                hits += 1
    return hits, residue


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--max-residue", type=int, default=0)
    parser.add_argument("--limit", type=int, default=40)
    args = parser.parse_args()

    if not os.path.exists(REPORT):
        print("No report. Run:")
        print("  python -m reccmp.tools.asmcmp --target HYPERX --json build/report.json --silent")
        return 1

    size_of = sizes()
    rows = []
    for entry in json.load(open(REPORT, encoding="utf-8"))["data"]:
        try:
            address = int(str(entry.get("address", "")), 16)
        except ValueError:
            continue
        if address >= GAME_CODE_LIMIT:
            continue
        share = float(entry.get("matching", 0))
        if share >= 1.0 or share <= 0.0:
            continue
        hits, residue = scan(entry)
        if not hits:
            continue
        rows.append((size_of.get(address, 0), residue, hits, share * 100,
                     address, str(entry.get("name", ""))))

    if args.max_residue:
        rows = [r for r in rows if r[1] <= args.max_residue]
    rows.sort(reverse=True)
    shown = rows[:args.limit]

    print("%-7s %-8s %-6s %-7s %-11s %s"
          % ("prize", "residue", "loads", "now", "address", "name"))
    for prize, residue, hits, share, address, name in shown:
        print("%-7d %-8d %-6d %6.2f  0x%08x  %s"
              % (prize, residue, hits, share, address, name))

    print()
    print("%d functions listed of %d missing an object load; %d bytes of prize"
          % (len(shown), len(rows), sum(r[0] for r in rows)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
