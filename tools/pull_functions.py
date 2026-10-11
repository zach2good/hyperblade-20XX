"""Refresh the names in symbols/functions.csv from Ghidra.

This reads a separate Ghidra project's database and never writes to it. The other tools work
against the CSV, so they do not need Ghidra running.

    python tools/pull_functions.py

Columns: address, name, size, convention, signature. Only the name changes, in its own column
and inside the signature. The sizes come from `tools/fix_row_extents.py`, since Ghidra's bodies
stop at the first funclet, and the signatures and conventions carry corrections the database
lacks. A row Ghidra has no function for keeps its name, and a function with no row is skipped.
"""

import csv
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ghidra_api

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT = os.path.join(ROOT, "symbols", "functions.csv")

# The declared name: the qualified identifier in front of the parameter list.
DECLARATOR = re.compile(r"[\w:~@$?]+(?=\s*\()")
LISTING = re.compile(r"^(?P<name>.+?) at (?P<address>[0-9a-fA-F]{8})$")


def read_rows():
    with open(OUTPUT, encoding="utf-8", newline="") as handle:
        return list(csv.reader(handle))


def list_all():
    """One call. list_functions ignores both limit and offset and returns everything, so
    paginating it would loop forever."""
    page = ghidra_api.get("list_functions")
    entries = []
    for line in page.splitlines():
        match = LISTING.match(line.strip())
        if match:
            entries.append((match.group("address").lower(), match.group("name")))
    return entries


def main():
    ghidra_api.check()
    entries = list_all()
    print("listed %d functions" % len(entries))

    rows = read_rows()
    # A placeholder name says nothing, so it never replaces a name the CSV already has.
    names = {address: name for address, name in entries if not name.startswith("FUN_")}

    changed = 0
    for row in rows[1:]:
        name = names.get(row[0][2:].lower(), row[1])
        signature = DECLARATOR.sub(lambda unused: name, row[4], count=1)
        if [name, signature] != [row[1], row[4]]:
            row[1] = name
            row[4] = signature
            changed += 1

    with open(OUTPUT, "w", newline="", encoding="utf-8") as handle:
        csv.writer(handle).writerows(rows)

    print("%d of %d rows changed" % (changed, len(rows) - 1))
    print("wrote %s" % OUTPUT)
    return 0


if __name__ == "__main__":
    sys.exit(main())
