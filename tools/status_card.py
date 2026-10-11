"""Draw progress/status.svg, the status card, from build/report.json.

The card shows the share of functions written, their average accuracy, and a bar for the
total progress, counted the way reccmp's asmcmp counts them. It is drawn here, not from
reccmp's own card template, and it carries no artwork.

    python -m reccmp.tools.asmcmp --target HYPERX --json build/report.json --silent
    python tools/status_card.py
    python tools/status_card.py --html    # also reccmp's HTML report, progress/status.html
"""
import argparse
import json
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPORT = os.path.join(ROOT, "build", "report.json")
SVG_OUT = os.path.join(ROOT, "progress", "status.svg")
HTML_OUT = os.path.join(ROOT, "progress", "status.html")
INVENTORY = os.path.join(ROOT, "symbols", "functions.csv")

FUNCTION = 1
BAR_W = 400


def stats(report, total):
    """(written, total, accuracy %, progress %), with an effective match counted as exact."""
    written = 0
    raw = 0.0
    for row in report["data"]:
        if row.get("type", FUNCTION) != FUNCTION or row.get("stub"):
            continue
        written += 1
        raw += 1.0 if row.get("effective") else row["matching"]
    total = max(total, written, 1)
    return written, total, 100.0 * raw / max(written, 1), 100.0 * raw / total


def card(written, total, accuracy, progress):
    filled = BAR_W * progress / 100.0
    return "\n".join([
        '<svg xmlns="http://www.w3.org/2000/svg" width="480" height="150" viewBox="0 0 480 150" '
        'font-family="ui-monospace,Consolas,monospace" font-weight="bold">',
        '<rect width="480" height="150" rx="8" fill="#1e293b"/>',
        '<text x="240" y="38" fill="#ffffff" font-size="22" text-anchor="middle">HYPERX.EXE</text>',
        '<text x="40" y="70" fill="#e2e8f0" font-size="14">'
        '<tspan>Implemented: %.2f%% (%d/%d)</tspan>'
        '<tspan x="40" dy="20">Accuracy:    %.2f%%</tspan></text>'
        % (100.0 * written / total, written, total, accuracy),
        '<rect x="40" y="106" width="%d" height="22" fill="#0f172a" stroke="#e2e8f0"/>' % BAR_W,
        '<rect x="40" y="106" width="%.1f" height="22" fill="#2563eb"/>' % filled,
        '<text x="240" y="122" fill="#ffffff" font-size="13" text-anchor="middle">'
        '<tspan>%.2f%%</tspan></text>' % progress,
        "</svg>",
        "",
    ])


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--html", action="store_true",
                        help="also write reccmp's HTML diff report to progress/status.html")
    parser.add_argument("--total", type=int, default=None,
                        help="total functions (default: the rows in symbols/functions.csv)")
    args = parser.parse_args()

    if not os.path.exists(REPORT):
        print("no %s; score the build with reccmp's asmcmp first" % os.path.relpath(REPORT, ROOT))
        return 2

    total = args.total
    if total is None:
        with open(INVENTORY, encoding="utf-8") as handle:
            total = sum(1 for _ in handle) - 1

    with open(REPORT, encoding="utf-8") as handle:
        figures = stats(json.load(handle), total)

    with open(SVG_OUT, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(card(*figures))
    print("wrote %s: %d/%d written, %.2f%% accurate, %.2f%% progress"
          % ((os.path.relpath(SVG_OUT, ROOT),) + figures))

    if args.html:
        return subprocess.run([sys.executable, "-m", "reccmp.tools.asmcmp", "--target", "HYPERX",
                               "--html", HTML_OUT, "--silent"], cwd=ROOT).returncode
    return 0


if __name__ == "__main__":
    sys.exit(main())
