"""Lists functions reccmp scores over a prefix because this build is shorter.

When reccmp does not know the original's function size, `compare_function` falls back to
`min(max_size(ORIG), size(RECOMP))`, which is this build's length. A port shorter than the
original is compared only as far as it goes, and the missing tail never appears in the diff.

A short port is often a complete one: cross-jumped case tails, folded switch arms, a tail call
or a merged block all make it shorter. Read the diff before calling a function unfinished.

    python tools/short_ports.py
"""

import contextlib
import csv
import io
import json
import logging
import os
import pathlib
import re
import sys

from reccmp.compare import Compare
from reccmp.compare.db import ImageId
from reccmp.project.detect import RecCmpProject

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPORT = os.path.join(ROOT, "build", "report.json")
INVENTORY = os.path.join(ROOT, "symbols", "functions.csv")
RETIRED = os.path.join(ROOT, "symbols", "retired.csv")
PLACEHOLDER = re.compile(r"^(FUN_|thunk_|LAB_|DAT_|SUB_|StaticDtor|Destructors::|Unwind@)")


def inventory():
    """Address to (size, name)."""
    rows = {}
    for row in csv.DictReader(
        line for line in open(INVENTORY, encoding="utf-8") if not line.startswith("#")
    ):
        rows[int(row["address"], 16)] = (int(row["size"]), row["name"])
    return rows


def used_sizes():
    logging.disable(logging.CRITICAL)
    project = RecCmpProject.from_directory(pathlib.Path(ROOT))
    noise = io.StringIO()
    with contextlib.redirect_stdout(noise):
        compare = Compare.from_target(project.get("HYPERX"))
    out = {}
    for match in compare._db.get_functions():
        size = match.size(ImageId.ORIG)
        if size is None:
            recomp = match.size(ImageId.RECOMP)
            if recomp is None:
                continue
            limit = match.max_size(ImageId.ORIG)
            size = recomp if limit is None else min(limit, recomp)
        out[match.orig_addr] = size
    return out


def main():
    rows = inventory()
    used = used_sizes()
    report = {}
    for entry in json.load(open(REPORT, encoding="utf-8"))["data"]:
        report[int(str(entry["address"]), 16)] = (
            float(entry.get("matching", 0)) * 100,
            str(entry.get("name", "")),
        )
    gone = set(int(line.split(",", 1)[0], 16)
               for line in open(RETIRED, encoding="utf-8") if line.startswith("0x"))
    listed = []
    for address, (declared, name) in rows.items():
        if address not in used or address not in report:
            continue
        if address in gone:
            continue
        share, reported = report[address]
        if share >= 100:
            continue
        if not name or PLACEHOLDER.match(name):
            name = reported or name
        missing = declared - used[address]
        if missing < 16:
            continue
        listed.append((missing, address, name, declared, used[address], share))
    listed.sort(reverse=True)
    print(
        "%d LIVE functions are scored over a prefix, %d original bytes never compared"
        " (retired rows excluded)"
        % (len(listed), sum(row[0] for row in listed))
    )
    for missing, address, name, declared, got, share in listed[:60]:
        print(
            "   %6.2f  0x%08x  %-42s orig %5d  compared %5d  short %5d"
            % (share, address, name, declared, got, missing)
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
