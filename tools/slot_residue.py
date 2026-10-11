"""Rank open rows by how much of their residue is a frame-slot displacement fault.

A displacement fault is a pair of lines, one on each side, that are the same instruction after
the `esp + 0xNN` or `ebp - 0xNN` constants are blanked and different before. Those are the rows
where the two builds compute the same thing out of different frame slots, which is what the
frame-aggregate lever reaches.

Blanking a displacement cannot tell a whole frame sliding from two slots trading places, so the
`kind` column separates them:

    size   the two builds disagree on `sub esp, K`, so every displacement moves with the frame.
           It outranks the other kinds because it is read off an instruction.
    perm   the deltas carry both signs, so slots trade places. This is the only kind a frame
           record can reach.
    push   one side has a callee-saved `push` the other does not, so the two builds stand at
           different stack depths: a live range, not a slot.
    shift  one sign and neither tell, so the cause is above the blanked window.

The tool reads build/report.json and costs no build.

    python tools/slot_residue.py                # ranked, worst first
    python tools/slot_residue.py --min-share 40 # only rows where it dominates
    python tools/slot_residue.py --retired      # the retired rows, with their walls

`--retired` ranks every row in `symbols/retired.csv` instead of the open ones, prints the wall
beside each, and includes rows with no frame-slot fault at all, which the frame lever cannot
reach.
"""
import csv
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPORT = os.path.join(ROOT, "build", "report.json")
RETIRED = os.path.join(ROOT, "symbols", "retired.csv")

ANNOT = re.compile(r"\s*\t\(.*\)$")
DISP = re.compile(r"\b(esp|ebp)\s*([+-])\s*(0x[0-9a-f]+|\d+)")
SUBESP = re.compile(r"^(sub|add)\s+esp\s*,")
SAVE = re.compile(r"^push\s+(ebx|ebp|esi|edi)$")


def body(text):
    return ANNOT.sub("", text).strip()


def blank(text):
    return DISP.sub(lambda m: m.group(1) + " ? ", text)


def displacements(text):
    return [(m.group(1), m.group(2), int(m.group(3), 16 if m.group(3).startswith("0x") else 10))
            for m in DISP.finditer(text)]


def kind_of(deltas, sized, pushed):
    """The kind of a displacement fault, read from the residue's own tells.

    A differing `sub esp` settles the row first. The push tell never overrides a permutation,
    because an argument push of a callee-saved register looks the same as a save of one.
    """
    if sized:
        return "size"
    if any(d > 0 for d in deltas) and any(d < 0 for d in deltas):
        return "perm"
    if pushed:
        return "push"
    return "shift"


def classify(entry):
    total = 0
    slot = 0
    deltas = {}
    sized = False
    pushed = False
    for group in entry.get("diff") or []:
        for chunk in group[1]:
            if "both" in chunk:
                continue
            orig = [body(line[1]) for line in chunk.get("orig", [])]
            recomp = [body(line[1]) for line in chunk.get("recomp", [])]
            total += len(orig) + len(recomp)
            if any(SUBESP.match(line) for line in orig + recomp):
                sized = True
            if (len([line for line in orig if SAVE.match(line)])
                    != len([line for line in recomp if SAVE.match(line)])):
                pushed = True
            if len(orig) != len(recomp):
                continue
            for a, b in zip(orig, recomp):
                if a == b or blank(a) != blank(b):
                    continue
                slot += 2
                da = displacements(a)
                db = displacements(b)
                for x, y in zip(da, db):
                    if x[0] != y[0] or x[1] != y[1]:
                        continue
                    d = y[2] - x[2]
                    deltas[d] = deltas.get(d, 0) + 1
    return total, slot, deltas, kind_of(deltas, sized, pushed)


def retired_walls():
    out = {}
    with open(RETIRED, encoding="utf-8") as handle:
        for row in csv.DictReader(line for line in handle if not line.startswith("#")):
            try:
                out[int(row["address"], 16)] = row.get("wall", "")
            except (KeyError, ValueError, TypeError):
                continue
    return out


def main(argv):
    minshare = 0.0
    if "--min-share" in argv:
        minshare = float(argv[argv.index("--min-share") + 1])
    walls = retired_walls() if "--retired" in argv else None
    report = json.load(open(REPORT))
    rows = []
    for entry in report["data"]:
        if entry.get("matching", 0) >= 1.0 or entry.get("stub"):
            continue
        if walls is not None and int(entry["address"], 16) not in walls:
            continue
        total, slot, deltas, kind = classify(entry)
        if not total:
            continue
        if not slot and walls is None:
            continue
        share = 100.0 * slot / total
        if share < minshare:
            continue
        rows.append((slot, share, total, entry, deltas, kind))
    rows.sort(key=lambda r: -r[0])
    tail = "wall" if walls is not None else "deltas"
    print("%-12s %-44s %6s %6s %6s %6s %5s  %s" % (
        "address", "name", "match", "slot", "total", "share", "kind", tail))
    for slot, share, total, entry, deltas, kind in rows:
        if walls is not None:
            last = walls[int(entry["address"], 16)]
        else:
            top = sorted(deltas.items(), key=lambda kv: -kv[1])[:4]
            last = " ".join("%+d:%d" % (d, n) for d, n in top)
        print("%-12s %-44s %6.2f %6d %6d %5.1f%% %5s  %s" % (
            entry["address"], entry["name"][:44], 100.0 * entry["matching"],
            slot, total, share, kind if slot else "", last))
    reached = len([r for r in rows if r[0]])
    perms = len([r for r in rows if r[0] and r[5] == "perm"])
    print("%d rows, %d with a frame-slot fault, %d of those a slot ORDER" % (
        len(rows), reached, perms))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
