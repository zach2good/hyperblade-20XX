# Report a ledger row whose recorded score the report no longer gives.
#
# symbols/retired.csv and symbols/blocked_rows.csv hold rows whose remaining gap was measured and
# refused, and tools/near_misses.py leaves them out. Each row's evidence column states its
# score. A drift can mean the row needs re-reading: a row that has gone up may no longer need to
# be retired, and a row that has gone down lost bytes to some later change.
#
# The evidence also quotes the scores of spellings that were tried, so only these shapes are read
# as the row's own figure, first match wins:
#
#   92.96: the client ...              a leading figure
#   373 bytes at 99.15, ...            a leading size and figure, also "82 bytes, 86.96."
#   ... at 94.68 ... / ... 45.87: ...  "at NN.NN" or "NN.NN:" in the first sentence
#   ... stores score 38.27             "score(s) NN.NN", "stands at" or "sits at" anywhere
#
# A row with none of these is not read. A drift under TOLERANCE is ignored, since an unrelated
# alignment change moves scores that much.
#
#   python tools/check_ledger_figures.py
#   python tools/check_ledger_figures.py --list    # also print every row read, and the tolerance
#
# Reads build/report.json and the two ledgers. Run the report first. Exit code 1 on any drift.

import csv
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPORT = os.path.join(ROOT, "build", "report.json")
LEDGERS = (os.path.join("symbols", "retired.csv"), os.path.join("symbols", "blocked_rows.csv"))

NUMBER = r"(\d{1,3}\.\d\d)(?!\d)"

LEADING = (
    re.compile(r'^\s*"?' + NUMBER),
    re.compile(r'^\s*"?\d+ bytes(?:,| at) ' + NUMBER),
)

FIRST_SENTENCE = (
    re.compile(r"\bat " + NUMBER),
    re.compile(NUMBER + r":"),
)

ANYWHERE = re.compile(r"\b(?:scores?|stands at|sits at) " + NUMBER)

SENTENCE_END = re.compile(r"\.(?:\s|$)")

TOLERANCE = 1.0


def scores():
    with open(REPORT, encoding="utf-8") as source:
        found = {}

        for entry in json.load(source)["data"]:
            try:
                found[int(str(entry.get("address")), 16)] = entry.get("matching")
            except (TypeError, ValueError):
                continue

        return found


def rows(path):
    with open(path, encoding="utf-8") as source:
        lines = [line for line in source if not line.startswith("#")]

    for row in csv.reader(lines):
        if len(row) < 4:
            continue

        try:
            yield int(row[0], 16), row[1], row[3]
        except ValueError:
            continue


def recorded_figure(evidence):
    for pattern in LEADING:
        found = pattern.match(evidence)

        if found:
            return float(found.group(1))

    end = SENTENCE_END.search(evidence)
    first = evidence

    if end:
        first = evidence[:end.start()]

    for pattern in FIRST_SENTENCE:
        found = pattern.search(first)

        if found:
            return float(found.group(1))

    found = ANYWHERE.search(evidence)

    if found:
        return float(found.group(1))

    return None


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    if not os.path.exists(REPORT):
        print("no report; run reccmp's asmcmp with --json build/report.json first")
        return 1

    now = scores()
    read = []
    faults = []

    for ledger in LEDGERS:
        for address, name, evidence in rows(ledger):
            recorded = recorded_figure(evidence)

            if recorded is None or now.get(address) is None:
                continue

            current = round(now[address] * 100, 2)
            entry = (ledger.replace(os.sep, "/"), address, name, recorded, current)
            read.append(entry)

            if abs(recorded - current) > TOLERANCE:
                faults.append(entry)

    if show:
        for ledger, address, name, recorded, current in read:
            print("READ   %-22s 0x%08x %-44s states %.2f, the report gives %.2f"
                  % (ledger, address, name, recorded, current))

    for ledger, address, name, recorded, current in faults:
        print("STALE  %-22s 0x%08x %-44s states %.2f, the report gives %.2f"
              % (ledger, address, name, recorded, current))

    if show:
        print("tolerance %.1f point(s)" % TOLERANCE)

    print("%d ledger row(s) state a score, %d more than %.0f point away from the report"
          % (len(read), len(faults), TOLERANCE))

    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
