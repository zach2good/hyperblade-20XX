"""Measure how much of the original this project reproduces, by bytes.

A count of functions weighs a three-byte stub the same as a six-hundred-byte function, so this
reports bytes. It warns on a stale report and refuses --record with one.

Definitions, so a number here can be checked:

  image bytes     the virtual size of .text in the original, 0xaf12b
  game bytes      the recorded size of every function below 0x004a0000, which is
                  roughly where the game's own code ends and the CRT begins
  matched         the recorded size of every function reccmp scores at exactly 100
  written         of those, the ones this project wrote, rather than ones that match
                  because we link the same LIBCMT.LIB and something named the address

Matched is not the same as decompiled: the CRT matches without being reverse engineered.
"Written" moves only when somebody works out what a function does. Matched CRT and reccmp's
effective-only matches are shown on their own lines, outside the share.

    python tools/progress.py            report
    python tools/progress.py --record   report and append a row to progress/history.csv
"""

import argparse
import csv
import datetime
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPORT = os.path.join(ROOT, "build", "report.json")

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from staleness import report_is_stale

INVENTORY = os.path.join(ROOT, "symbols", "functions.csv")
HISTORY = os.path.join(ROOT, "progress", "history.csv")
SOURCE_ROOT = os.path.join(ROOT, "src")
IMAGE = os.path.join(ROOT, "build", "out", "HYPERX_sym.exe")

TEXT_VIRTUAL_SIZE = 0xAF12B
GAME_CODE_LIMIT = 0x004A0000

MARKER = re.compile(r"//\s*FUNCTION:\s*HYPERX\s+(0x[0-9a-fA-F]+)")


def written_addresses():
    """Every address this project claims with a FUNCTION marker."""
    found = set()
    for directory, _, names in os.walk(SOURCE_ROOT):
        for name in names:
            if not name.endswith((".cpp", ".c", ".h")):
                continue
            path = os.path.join(directory, name)
            with open(path, encoding="utf-8", errors="replace") as handle:
                for line in handle:
                    match = MARKER.search(line)
                    if match:
                        found.add(int(match.group(1), 16))
    return found


def load_sizes():
    sizes = {}
    with open(INVENTORY, encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            if str(row["size"]).isdigit():
                sizes[int(row["address"], 16)] = int(row["size"])
    return sizes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", action="store_true",
                        help="append a row to progress/history.csv")
    args = parser.parse_args()

    if not os.path.exists(REPORT):
        print("No report. Run:")
        print("  python -m reccmp.tools.asmcmp --target HYPERX --json build/report.json --silent")
        return 1

    late = report_is_stale(ROOT, IMAGE, REPORT)

    if late and args.record:
        print("REFUSED: the report does not describe the tree as it stands, so recording a row")
        print("would write the PREVIOUS build's figures into progress/history.csv. That has")
        print("happened: a row had to be deleted by hand afterwards. Build, measure, then record.")
        for why in late[:5]:
            print("    %s" % why)
        return 1

    if os.path.exists(IMAGE) and os.path.getmtime(IMAGE) > os.path.getmtime(REPORT):
        print("STALE REPORT: the image is newer than build/report.json.")
        print("Every figure below is the PREVIOUS build. Regenerate first:")
        print("  python -m reccmp.tools.asmcmp --target HYPERX --json build/report.json --silent")
        print()

    sizes = load_sizes()
    written = written_addresses()
    rows = json.load(open(REPORT, encoding="utf-8"))["data"]

    game_bytes = sum(size for address, size in sizes.items() if address < GAME_CODE_LIMIT)
    game_count = sum(1 for address in sizes if address < GAME_CODE_LIMIT)

    matched_bytes = 0
    matched_count = 0
    written_bytes = 0
    written_count = 0
    partial = []

    crt_bytes = 0
    crt_count = 0

    effective_bytes = 0
    effective_count = 0

    for row in rows:
        address = int(row["address"], 16)
        size = sizes.get(address, 0)

        if row["matching"] >= 1.0:
            # The share is of game code, so matched CRT goes on its own line.
            if address >= GAME_CODE_LIMIT:
                crt_bytes += size
                crt_count += 1
                continue

            matched_bytes += size
            matched_count += 1

            if address in written:
                written_bytes += size
                written_count += 1
        else:
            # An effective match differs only in register names or unrenderable operands. It is
            # the client's code but not its bytes, so it stays outside the share.
            if row.get("effective") and address < GAME_CODE_LIMIT:
                effective_bytes += size
                effective_count += 1

            if address in written:
                partial.append((row["matching"], address, row["name"], size))

    def share(part, whole):
        if not whole:
            return 0.0
        return 100.0 * part / whole

    print("%-42s %10s %9s" % ("", "bytes", "of game"))
    print("%-42s %10d %8.2f%%" % ("game code below 0x004a0000", game_bytes, 100.0))
    print("%-42s %10d %8.2f%%" % ("matched exactly", matched_bytes, share(matched_bytes, game_bytes)))
    print("%-42s %10d %8.2f%%" % ("of those, written here", written_bytes, share(written_bytes, game_bytes)))
    print("%-42s %10d %9s" % ("CRT matched, outside the share", crt_bytes, "-"))
    print("%-42s %10d %9s" % ("effective only, outside the share", effective_bytes, "-"))
    print()
    print("functions below 0x004a0000 : %d" % game_count)
    print("matching exactly           : %d" % matched_count)
    print("CRT matching, not counted  : %d" % crt_count)
    print("effective, not exact       : %d" % effective_count)
    print("written here and matching  : %d" % written_count)
    print(".text virtual size         : %d bytes, so game code is %.1f%% of it"
          % (TEXT_VIRTUAL_SIZE, share(game_bytes, TEXT_VIRTUAL_SIZE)))

    if partial:
        print()
        print("written but not matching:")
        for ratio, address, name, size in sorted(partial):
            print("  %6.2f%%  0x%08x  %-40s %d bytes" % (100.0 * ratio, address, name[:40], size))

    if args.record:
        os.makedirs(os.path.dirname(HISTORY), exist_ok=True)
        fresh = not os.path.exists(HISTORY)
        with open(HISTORY, "a", newline="", encoding="utf-8") as handle:
            writer = csv.writer(handle)
            if fresh:
                writer.writerow(["date", "game_bytes", "matched_bytes", "written_bytes",
                                 "matched_count", "written_count"])
            writer.writerow([datetime.date.today().isoformat(), game_bytes, matched_bytes,
                             written_bytes, matched_count, written_count])
        print()
        print("appended to %s" % HISTORY)

    return 0


if __name__ == "__main__":
    sys.exit(main())
