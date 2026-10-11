"""Reports a README whose status figures no longer match the report.

It checks the bold headline and every row of the status table in README.md against
build/report.json and symbols/functions.csv, counted as tools/progress.py counts them. A table
row it does not recognise is a fault too, so no figure goes unchecked. Every figure must be exact,
the share to the two decimals the README prints. It refuses to run on a stale report.

Informational by default. Pass --strict when updating the README, to fail on drift.

    python tools/check_readme.py            # report drift, always exit 0
    python tools/check_readme.py --strict   # exit non-zero on drift
"""
import csv
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPORT = os.path.join(ROOT, "build", "report.json")
IMAGE = os.path.join(ROOT, "build", "out", "HYPERX_sym.exe")

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from staleness import report_is_stale

INVENTORY = os.path.join(ROOT, "symbols", "functions.csv")
README = os.path.join(ROOT, "README.md")

GAME_CODE_LIMIT = 0x004A0000

# Each status table row, by the start of its label, and the figures its value cell holds.
TABLE = (
    ("Game functions", ("game_count",)),
    ("**Matching exactly**", ("exact",)),
    ("Effective but not exact", ("effective",)),
    ("Exact bytes of game code", ("exact_bytes", "game_bytes")),
    ("Rows in `symbols/functions.csv`", ("rows",)),
)

NUMBER = re.compile(r"\d[\d,]*(?:\.\d+)?")


def inventory_sizes():
    sizes = {}
    rows = 0
    with open(INVENTORY, encoding="utf-8") as handle:
        for row in csv.DictReader(line for line in handle if not line.startswith("#")):
            try:
                address = int(row["address"], 16)
            except (KeyError, ValueError, TypeError):
                continue
            rows += 1
            if str(row.get("size", "")).isdigit():
                sizes[address] = int(row["size"])
    return sizes, rows


def measured():
    sizes, rows = inventory_sizes()
    out = {
        "rows": rows,
        "game_count": sum(1 for a in sizes if a < GAME_CODE_LIMIT),
        "game_bytes": sum(s for a, s in sizes.items() if a < GAME_CODE_LIMIT),
        "exact": 0,
        "exact_bytes": 0,
        "effective": 0,
    }

    for row in json.load(open(REPORT, encoding="utf-8"))["data"]:
        try:
            address = int(str(row.get("address", "")), 16)
        except ValueError:
            continue
        if address >= GAME_CODE_LIMIT:
            continue
        if float(row.get("matching", 0)) >= 1.0:
            out["exact"] += 1
            out["exact_bytes"] += sizes.get(address, 0)
        elif row.get("effective"):
            out["effective"] += 1

    game_bytes = out["game_bytes"]
    out["share"] = 100.0 * out["exact_bytes"] / game_bytes if game_bytes else 0.0
    return out


def figure(text):
    return float(text.replace(",", ""))


def stated(text):
    """The figures the README asserts, and the table rows it cannot read."""
    says = []
    unknown = []

    hit = re.search(r"\*\*([\d,]+) of ([\d,]+) game functions match exactly: ([\d,]+) bytes, "
                    r"([\d.]+) per cent of game code\.\*\*", text)
    if hit:
        for key, group in zip(("exact", "game_count", "exact_bytes", "share"), hit.groups()):
            says.append(("headline", key, figure(group)))

    start = text.find("| Measure | Value |")
    if start < 0:
        return says, unknown, False

    for line in text[start:].splitlines()[2:]:
        if not line.startswith("|"):
            break
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) != 2:
            unknown.append(line)
            continue
        label, value = cells
        keys = next((k for prefix, k in TABLE if label.startswith(prefix)), None)
        numbers = NUMBER.findall(value)
        if keys is None or len(numbers) != len(keys):
            unknown.append(line)
            continue
        for key, number in zip(keys, numbers):
            says.append((label, key, figure(number)))

    return says, unknown, True


def main():
    strict = "--strict" in sys.argv
    if not os.path.exists(REPORT):
        print("No report. Run:")
        print("  python -m reccmp.tools.asmcmp --target HYPERX --json build/report.json --silent")
        return 1

    late = report_is_stale(ROOT, IMAGE, REPORT)

    if late:
        print("REFUSED: the report does not describe the tree as it stands, so the figures below")
        print("would be the PREVIOUS build's. Build and measure, then run this.")
        for why in late[:5]:
            print("    %s" % why)
        return 1

    have = measured()
    text = open(README, encoding="utf-8").read()
    says, unknown, has_table = stated(text)

    bad = []
    if not has_table:
        bad.append("no status table (\"| Measure | Value |\")")
    if not any(where == "headline" for where, key, value in says):
        bad.append("no bold headline figure")
    for line in unknown:
        bad.append("a status row this cannot check: %s" % line.strip())
    for where, key, value in says:
        if value != round(have[key], 2):
            bad.append("%s says %s %g, measured %s" % (where, key, value, round(have[key], 2)))

    for line in bad:
        print("STALE README: %s" % line)

    print("README checked %d figure(s); measured %d exact of %d, %d bytes, %.2f per cent, "
          "%d effective" % (len(says), have["exact"], have["game_count"], have["exact_bytes"],
                            have["share"], have["effective"]))
    if bad and not strict:
        print("  informational only; pass --strict to fail on this")
    return 1 if bad and strict else 0


if __name__ == "__main__":
    sys.exit(main())
