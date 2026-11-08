"""Draw progress/status.svg, the status card, from build/report.json.

The card counts game functions only: rows of symbols/functions.csv below 0x004a0000, less the
LIBRARY rows (LIBCMT and IJG JPEG) and the unwritten C runtime statics at or above the lowest
LIBCMT marker. Its bar is the share that match exactly, and its second line the average reccmp
score of the game functions written, with an effective match counted as exact. It is drawn here,
not from reccmp's own card template, and it carries no artwork.

    python -m reccmp.tools.asmcmp --target HYPERX --json build/report.json --silent
    python tools/status_card.py
    python tools/status_card.py --html    # also reccmp's HTML report, progress/status.html
"""
import argparse
import csv
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPORT = os.path.join(ROOT, "build", "report.json")
SVG_OUT = os.path.join(ROOT, "progress", "status.svg")
HTML_OUT = os.path.join(ROOT, "progress", "status.html")
INVENTORY = os.path.join(ROOT, "symbols", "functions.csv")
SOURCE_ROOT = os.path.join(ROOT, "src")
CRT_MARKERS = os.path.join(SOURCE_ROOT, "common", "library.h")

FUNCTION = 1
GAME_CODE_LIMIT = 0x004A0000
BAR_W = 400

LIBRARY = re.compile(r"//\s*LIBRARY:\s*HYPERX\s+(0x[0-9a-fA-F]+)")


def library_addresses():
    """Every LIBRARY address, and the lowest one in the LIBCMT header."""
    found = set()
    lowest_crt = GAME_CODE_LIMIT
    for directory, _, names in os.walk(SOURCE_ROOT):
        for name in names:
            if not name.endswith((".cpp", ".c", ".h")):
                continue
            path = os.path.join(directory, name)
            with open(path, encoding="utf-8", errors="replace") as handle:
                for address in LIBRARY.findall(handle.read()):
                    found.add(int(address, 16))
                    if os.path.samefile(path, CRT_MARKERS):
                        lowest_crt = min(lowest_crt, int(address, 16))
    return found, lowest_crt


def game_figures(report):
    """(exact, game functions, accuracy % of the written ones)."""
    scores = {}
    for row in report["data"]:
        if row.get("type", FUNCTION) != FUNCTION or row.get("stub"):
            continue
        scores[int(row["address"], 16)] = 1.0 if row.get("effective") else row["matching"]
    library, lowest_crt = library_addresses()
    exact = 0
    total = 0
    written = 0
    raw = 0.0
    with open(INVENTORY, encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            address = int(row["address"], 16)
            if address >= GAME_CODE_LIMIT or address in library:
                continue
            if address not in scores:
                if address >= lowest_crt:
                    continue
                total += 1
                continue
            total += 1
            written += 1
            raw += scores[address]
            if scores[address] >= 1.0:
                exact += 1
    return exact, max(total, 1), 100.0 * raw / max(written, 1)


def card(exact, total, accuracy):
    share = 100.0 * exact / total
    filled = BAR_W * share / 100.0
    return "\n".join([
        '<svg xmlns="http://www.w3.org/2000/svg" width="480" height="150" viewBox="0 0 480 150" '
        'font-family="ui-monospace,Consolas,monospace" font-weight="bold">',
        '<rect width="480" height="150" rx="8" fill="#1e293b"/>',
        '<text x="240" y="38" fill="#ffffff" font-size="22" text-anchor="middle">HYPERX.EXE</text>',
        '<text x="40" y="70" fill="#e2e8f0" font-size="14">'
        '<tspan>Exact:    %.2f%% (%d/%d game functions)</tspan>'
        '<tspan x="40" dy="20">Accuracy: %.2f%%</tspan></text>'
        % (share, exact, total, accuracy),
        '<rect x="40" y="106" width="%d" height="22" fill="#0f172a" stroke="#e2e8f0"/>' % BAR_W,
        '<rect x="40" y="106" width="%.1f" height="22" fill="#2563eb"/>' % filled,
        '<text x="240" y="122" fill="#ffffff" font-size="13" text-anchor="middle">'
        '<tspan>%.2f%%</tspan></text>' % share,
        "</svg>",
        "",
    ])


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--html", action="store_true",
                        help="also write reccmp's HTML diff report to progress/status.html")
    args = parser.parse_args()

    if not os.path.exists(REPORT):
        print("no %s; score the build with reccmp's asmcmp first" % os.path.relpath(REPORT, ROOT))
        return 2

    with open(REPORT, encoding="utf-8") as handle:
        report = json.load(handle)

    figures = game_figures(report)

    with open(SVG_OUT, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(card(*figures))
    print("wrote %s: %d/%d game functions exact, %.2f%% accurate"
          % ((os.path.relpath(SVG_OUT, ROOT),) + figures))

    if args.html:
        return subprocess.run([sys.executable, "-m", "reccmp.tools.asmcmp", "--target", "HYPERX",
                               "--html", HTML_OUT, "--silent"], cwd=ROOT).returncode
    return 0


if __name__ == "__main__":
    sys.exit(main())
