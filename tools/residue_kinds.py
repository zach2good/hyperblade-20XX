"""Split a row's residue into register or constant naming and a different instruction stream.

Every general register is normalised to one token and every literal to another, and the
orig-only lines are matched against the recomp-only ones as a multiset:

    rename     a normalised line present on both sides. One value, another register.
    structural a normalised line present on one side only. A different instruction stream,
               which is what source can reach.

It reads build/report.json and costs no build.

    python tools/residue_kinds.py 0x0046f530          # one row, with the structural lines
    python tools/residue_kinds.py --band 0.50 0.80    # every open row in a band, ranked
"""
import argparse
import collections
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPORT = os.path.join(ROOT, "build", "report.json")
sys.path.insert(0, os.path.join(ROOT, "tools"))
import near_misses

GAME_CODE_LIMIT = 0x004A0000

REG32 = re.compile(r"\b(?:eax|ebx|ecx|edx|esi|edi|ebp)\b")
REG16 = re.compile(r"\b(?:ax|bx|cx|dx|si|di)\b")
REG8 = re.compile(r"\b(?:al|bl|cl|dl|ah|bh|ch|dh)\b")
NUM = re.compile(r"\b(?:0x[0-9a-f]+|\d+)\b")


def normalise(line):
    text = line.split("\t")[0].strip()
    text = REG32.sub("R", text)
    text = REG16.sub("W", text)
    text = REG8.sub("B", text)
    return NUM.sub("K", text)


def residue(entry):
    """The orig-only and recomp-only instruction text of one row, in order."""
    orig = []
    recomp = []
    for block in entry.get("diff") or []:
        for group in block[1]:
            for line in group.get("orig") or []:
                orig.append(line[1])
            for line in group.get("recomp") or []:
                recomp.append(line[1])
    return orig, recomp


def split(orig, recomp):
    counted_orig = collections.Counter(normalise(x) for x in orig)
    counted_recomp = collections.Counter(normalise(x) for x in recomp)
    shared = counted_orig & counted_recomp
    renames = sum(shared.values())
    return renames, counted_orig - counted_recomp, counted_recomp - counted_orig


def load():
    with open(REPORT) as handle:
        return json.load(handle)["data"]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("address", nargs="?")
    parser.add_argument("--band", nargs=2, type=float, metavar=("LOW", "HIGH"))
    parser.add_argument("--min-bytes", type=int, default=0)
    args = parser.parse_args()

    if not os.path.exists(REPORT):
        print("no build/report.json; run asmcmp --json first")
        return 2

    rows = load()
    sizes = near_misses.sizes()
    shut = near_misses.retired()

    if args.address:
        want = int(args.address, 16)
        for entry in rows:
            if int(entry["address"], 16) != want:
                continue
            orig, recomp = residue(entry)
            renames, only_orig, only_recomp = split(orig, recomp)
            total = len(orig) + len(recomp)
            structural = sum(only_orig.values()) + sum(only_recomp.values())
            print("%s %s" % (entry["address"], entry.get("name", "")))
            print("residue %d lines: %d rename, %d structural" % (total, renames * 2, structural))
            print("\nORIG ONLY")
            for text, count in only_orig.most_common():
                print("  %2d  %s" % (count, text))
            print("\nOURS ONLY")
            for text, count in only_recomp.most_common():
                print("  %2d  %s" % (count, text))
            return 0
        print("no row at %s" % args.address)
        return 1

    low, high = args.band or (0.50, 0.80)
    ranked = []
    for entry in rows:
        address = int(entry["address"], 16)
        matching = entry.get("matching") or 0.0
        if address >= GAME_CODE_LIMIT or address in shut:
            continue
        if not (low <= matching < high):
            continue
        size = sizes.get(address, 0)
        if size < args.min_bytes:
            continue
        orig, recomp = residue(entry)
        renames, only_orig, only_recomp = split(orig, recomp)
        total = len(orig) + len(recomp)
        structural = sum(only_orig.values()) + sum(only_recomp.values())
        share = 0.0
        if total:
            share = float(structural) / total
        ranked.append((size, matching, address, entry.get("name", ""), total, structural, share))

    ranked.sort(key=lambda row: -row[0])
    print("%5s %6s %-10s %5s %5s %5s  %s" % ("bytes", "match", "address", "res", "struc", "share", "name"))
    for size, matching, address, name, total, structural, share in ranked:
        print("%5d %6.2f 0x%08x %5d %5d %5.0f%%  %s"
              % (size, matching * 100.0, address, total, structural, share * 100.0, name))
    return 0


if __name__ == "__main__":
    sys.exit(main())
