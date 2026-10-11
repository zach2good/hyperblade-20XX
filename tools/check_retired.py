"""Guard the retirement registers against duplicates and stale names.

`symbols/retired.csv` is `merge=union` in `.gitattributes`, so two branches that append
rows do not conflict, but a row added on both survives twice and doubles its weight in any
count. A row's name is the function's name in the tree, so a rename or a move to another class
leaves it stale; `symbols/blocked_rows.csv` is held to the same rule. Exits 1 on a duplicate
address or a name that disagrees with the FUNCTION marker at that address. An address with no
marker in src/ is a library body and is skipped.

    python tools/check_retired.py
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import check_named_addresses

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

REGISTERS = ("symbols/retired.csv", "symbols/blocked_rows.csv")


def main():
    os.chdir(ROOT)
    owner = check_named_addresses.owners()
    duplicates = []
    stale = []
    rows = 0

    for register in REGISTERS:
        seen = {}
        lines = open(register, encoding="utf-8").read().split("\n")

        for number, line in enumerate(lines, 1):
            if not line.startswith("0x"):
                continue

            rows += 1
            fields = line.split(",", 2)
            address = fields[0].lower()

            if address in seen:
                duplicates.append((register, address, seen[address], number))
            else:
                seen[address] = number

            real = owner.get(address)

            if real is not None and fields[1] != real:
                stale.append((register, number, address, fields[1], real))

    for register, address, first, second in duplicates:
        print("DUPLICATE %s %s  lines %d and %d" % (register, address, first, second))

    for register, number, address, named, real in stale:
        print("STALE NAME %s:%d %s names %s, the tree has %s" % (register, number, address, named, real))

    print("%d row(s), %d duplicate address(es), %d stale name(s)" % (rows, len(duplicates), len(stale)))
    return 1 if duplicates or stale else 0


if __name__ == "__main__":
    sys.exit(main())
