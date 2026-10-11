"""Compares the current report against a saved baseline and names what stopped matching.

Some changes cost exact matches in functions nobody touched, for example a GLOBAL marker
declared inside an existing global. The build is clean and every marker checker passes; only
the total drops.

    python tools/check_regressions.py --save        # before touching anything
    ...work...
    python -m reccmp.tools.asmcmp --target HYPERX --json build/report.json --silent
    python tools/check_regressions.py               # after

It reports functions that vanished from the report, fell out of an exact match, reached one,
are new since the baseline, or moved by more than a point. It also reports a retired or blocked
row that is now exact. It exits 1 on a lost exact match, a vanished row, any row that dropped by
more than a point, a stale retirement, or a stale build.

`--save` refuses while the current report has a row more than a point below the baseline, or has
lost one, since saving then would hide the drop from the next comparison. Pass `--accept-drops`
with `--save` only when the drop has been read and is meant.

The baseline lives beside the report and is not committed.
"""
import json
import os
import sys

REPORT = os.path.join("build", "report.json")
BASELINE = os.path.join("build", "report_baseline.json")

EXACT = 0.999
NOTABLE = 0.01


def load(path):
    with open(path) as handle:
        return {row["address"]: row for row in json.load(handle)["data"]}


IMAGE = os.path.join("build", "out", "HYPERX_sym.exe")


def stale_sources():
    """Source files newer than the linked image the report was made from.

    A source edited after the last link shifts its line numbers against the PDB, and
    reccmp can drop its functions from the report, which reads as VANISHED.

    The image is HYPERX_sym.exe, which `--target HYPERX` does not rebuild. Build the
    default target, `cmake --build build`.
    """
    if not os.path.exists(IMAGE):
        return []
    linked = os.path.getmtime(IMAGE)
    late = []
    for root, _, files in os.walk("src"):
        for name in files:
            if not name.endswith((".c", ".cpp", ".h")):
                continue
            path = os.path.join(root, name)
            if os.path.getmtime(path) > linked:
                late.append(path)
    return late


if not os.path.exists(REPORT):
    print("%s missing; run asmcmp --json first" % REPORT)
    sys.exit(1)

STALE = stale_sources()
if STALE and "--save" not in sys.argv[1:]:
    print("STALE BUILD: %d source file(s) are newer than %s." % (len(STALE), IMAGE))
    for path in STALE[:5]:
        print("  %s" % path)
    if len(STALE) > 5:
        print("  ... and %d more" % (len(STALE) - 5))
    print("REBUILD before believing any VANISHED line below: a source edited after the")
    print("last link can drop its function from the report and read as a regression.")


def drops(before, after):
    """Rows of `before` that `after` lost or scores more than a point lower."""
    fallen = []

    for address, was in before.items():
        old = was.get("matching", 0)
        row = after.get(address)

        if row is None:
            fallen.append((address, was["name"], old, None))
            continue

        new = row.get("matching", 0)

        if old - new > NOTABLE or (old >= EXACT and new < EXACT):
            fallen.append((address, row["name"], old, new))

    return fallen


if "--save" in sys.argv[1:]:
    with open(REPORT) as source:
        text = source.read()

    if os.path.exists(BASELINE) and "--accept-drops" not in sys.argv[1:]:
        FALLEN = drops(load(BASELINE), load(REPORT))

        if FALLEN:
            for address, name, old, new in sorted(FALLEN):
                if new is None:
                    print("DROPPED     %-10s %-44s %.2f%% -> gone" % (address, name[:44], old * 100))
                    continue

                print("DROPPED     %-10s %-44s %.2f%% -> %.2f%%"
                      % (address, name[:44], old * 100, new * 100))

            print("REFUSED: %d row(s) are more than a point below the baseline or gone. Restore them,"
                  " or pass --accept-drops with --save if the drop is meant." % len(FALLEN))
            sys.exit(1)

    with open(BASELINE, "w") as target:
        target.write(text)

    print("baseline saved from %s" % REPORT)
    sys.exit(0)

if not os.path.exists(BASELINE):
    print("no baseline; run with --save before making changes")
    sys.exit(1)

RETIRED = os.path.join("symbols", "retired.csv")

# A blocked row is excluded from work lists like a retired one, so both registers are read.
BLOCKED = os.path.join("symbols", "blocked_rows.csv")


def stale_retirements(report):
    """Retired functions that are now exact.

    A stale row hides the function from every list that reads the register, so it should
    be removed.
    """
    registers = [path for path in (RETIRED, BLOCKED) if os.path.exists(path)]

    if not registers:
        return []
    scores = {}
    for row in report.values():
        try:
            scores[int(str(row.get("address", "")), 16)] = float(row.get("matching", 0))
        except ValueError:
            continue
    stale = []
    for register in registers:
      with open(register) as handle:
          for line in handle:
              if line.startswith("#") or line.startswith("address,") or not line.strip():
                  continue
              parts = line.split(",")
              try:
                  address = int(parts[0], 16)
              except ValueError:
                  continue
              if scores.get(address, 0) >= EXACT:
                  stale.append((parts[0], parts[1] if len(parts) > 1 else ""))
    return stale


before = load(BASELINE)
after = load(REPORT)

lost = []
gained = []
moved = []
added = []
gone = []

for address, row in after.items():
    was = before.get(address)

    if was is None:
        added.append((address, row["name"], row.get("matching", 0)))
        continue

    old = was.get("matching", 0)
    new = row.get("matching", 0)

    if old >= EXACT and new < EXACT:
        lost.append((address, row["name"], new))
    elif old < EXACT and new >= EXACT:
        gained.append((address, row["name"]))
    elif abs(new - old) > NOTABLE:
        moved.append((address, row["name"], old, new))

# An address missing from the new report, for example because its marker was deleted.
for address, was in before.items():
    if address not in after:
        gone.append((address, was["name"], was.get("matching", 0)))

for address, name, score in sorted(gone):
    print("VANISHED    %-10s %-44s was %.2f%%" % (address, name[:44], score * 100))

for address, name, score in sorted(lost):
    print("LOST EXACT  %-10s %-44s now %.2f%%" % (address, name[:44], score * 100))

for address, name in sorted(gained):
    print("NOW EXACT   %-10s %s" % (address, name[:44]))

for address, name, score in sorted(added):
    print("NEW         %-10s %-44s at %.2f%%" % (address, name[:44], score * 100))

for address, name, old, new in sorted(moved, key=lambda row: row[3] - row[2]):
    print("MOVED       %-10s %-44s %.2f%% -> %.2f%%" % (address, name[:44], old * 100, new * 100))

vanished_exact = [row for row in gone if row[2] >= EXACT]

# Every partial row that fell, beside the exact rows counted as lost.
dropped = [row for row in moved if row[3] < row[2]]

STALE_RETIRED = stale_retirements(after)
for address, name in STALE_RETIRED:
    print("STALE RETIREMENT %s %s is now exact; remove its row from %s"
          % (address, name, "symbols/retired.csv or symbols/blocked_rows.csv"))

# The last line repeats the staleness warning, so `tail` still shows it.
SUMMARY = ("%d lost, %d vanished (%d of them exact), %d gained, %d new, %d moved by more than a point,"
           " %d dropped by more than a point" % (
               len(lost), len(gone), len(vanished_exact), len(gained), len(added), len(moved),
               len(dropped)))

if STALE:
    SUMMARY = SUMMARY + "  [STALE BUILD, %d source(s) newer than the link: THIS COUNT IS NOT A MEASUREMENT]" % len(STALE)

print(SUMMARY)
sys.exit(1 if lost or gone or dropped or STALE_RETIRED or STALE else 0)
