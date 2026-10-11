"""Recover the class hierarchy from HYPERX.EXE's own vtables.

VC6 lays a derived class's table out as a copy of its base's with the overridden slots
replaced, so two tables agreeing on a prefix of slots share a base and the length of the
agreement is that base's virtual count.

A table's length is the gap to the next table, not a fixed slot count. Reading past the real
end walks into the next table and makes every class look like it overrides everything.

    python tools/vtable_forest.py              # the families, widest first
    python tools/vtable_forest.py --slots      # every table's slots, resolved
    python tools/vtable_forest.py --base 0x004b3d0c   # one table against its family

Needs a copy of the original binary, so it is a tool and not a checker.
"""
import argparse
import csv
import os
import re
import struct
import sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from peinfo import PEFile
from reference import reference_path

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INVENTORY = os.path.join(ROOT, "symbols", "functions.csv")

# A slot holds a code address; the first entry outside .text ends the table.
TEXT_LOW = 0x00401000
TEXT_HIGH = 0x004a0000


def sourceFiles(suffixes):
    """Every source file under src/, at any depth."""
    for where, _, names in os.walk(os.path.join(ROOT, "src")):
        for name in sorted(names):
            if name.endswith(suffixes):
                yield os.path.join(where, name)

def load_names():
    names = {}

    with open(INVENTORY, encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            names[int(row["address"], 16)] = row["name"]

    return names


def load_tables(image, names):
    # Every address anything in src/ calls a vtable bounds the table before it, not only the
    # placeholder tables the arity header names.
    known = set()

    for path in sourceFiles((".cpp", ".h")):
        text = open(path, encoding="utf-8", errors="replace").read()

        for hit in re.findall(r"VTABLE: HYPERX (0x004[0-9a-fA-F]{5})", text):
            known.add(int(hit, 16))

        for hit in re.findall(r"g_vtbl(004[bB][0-9a-fA-F]{4})", text):
            known.add(int(hit, 16))

    known = sorted(known)

    tables = {}

    for index, address in enumerate(known):
        if index + 1 < len(known):
            limit = (known[index + 1] - address) // 4
        else:
            limit = 64

        slots = image.read_dwords(address, min(limit, 64))

        count = 0
        while count < len(slots) and TEXT_LOW <= slots[count] < TEXT_HIGH:
            count += 1

        if count:
            tables[address] = [names.get(slot, "sub_%08x" % slot) for slot in slots[:count]]

    return tables


class Image:
    def __init__(self, path):
        self.pe = PEFile(path)

    def read_dwords(self, address, count):
        for section in self.pe.sections:
            start = self.pe.image_base + section["vaddr"]

            if start <= address < start + section["rawsize"]:
                offset = section["rawoff"] + (address - start)
                available = (section["rawoff"] + section["rawsize"] - offset) // 4

                return list(struct.unpack_from("<%dI" % min(count, available),
                                               self.pe.data, offset))

        return []


def families(tables):
    """Return [(members, base arity)], each a maximal set agreeing on a prefix.

    A one-slot agreement is every pair of tables in the image, so the walk starts at two.
    """
    groups = defaultdict(set)

    for address, slots in tables.items():
        for length in range(2, len(slots) + 1):
            groups[tuple(slots[1:length])].add(address)

    widest = {}

    for signature, members in groups.items():
        if len(members) < 2:
            continue

        key = frozenset(members)

        if key not in widest or len(signature) > len(widest[key]):
            widest[key] = signature

    ordered = sorted(widest.items(), key=lambda pair: (-len(pair[0]), -len(pair[1])))

    kept = []

    for members, signature in ordered:
        # A subset of a family already printed is a sub-branch, reported as its own entry.
        if any(members <= earlier for earlier, _ in kept):
            continue

        kept.append((members, signature))

    return [(sorted(members), len(signature) + 1) for members, signature in kept]


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--slots", action="store_true", help="print every table's slots")
    parser.add_argument("--base", help="print one table against the family it belongs to")
    args = parser.parse_args()

    original = reference_path(required=False)

    if original is None:
        print("no copy of the original is configured, so the tables cannot be read.")
        print("Name yours in reccmp-user.yml and run this again.")

        return 0

    image = Image(original)
    names = load_names()
    tables = load_tables(image, names)

    print("%d tables, %d slots" % (len(tables), sum(len(slots) for slots in tables.values())))

    if args.slots:
        for address in sorted(tables):
            print("\n0x%08x  %d slots" % (address, len(tables[address])))

            for index, name in enumerate(tables[address]):
                print("  %3d  %s" % (index, name))

        return 0

    found = families(tables)

    if args.base:
        wanted = int(args.base, 16)

        for members, arity in found:
            if wanted not in members:
                continue

            base = tables[wanted]

            print("\n0x%08x is one of %d classes over a %d-virtual base." % (wanted,
                                                                            len(members), arity))
            print("\nslot  %-44s overridden by" % "as 0x%08x has it" % wanted)

            for index in range(1, len(base)):
                differs = [tables[other][0] for other in members
                           if index < len(tables[other]) and tables[other][index] != base[index]]

                print("%4d  %-44s %s" % (index, base[index], ", ".join(differs) or "-"))

            return 0

        print("0x%s is in no family: nothing else shares a slot with it." % args.base)

        return 1

    for members, arity in found:
        print("\n%2d classes over a %d-virtual base:" % (len(members), arity))

        for address in members:
            print("      %-42s 0x%08x  %2d slots"
                  % (tables[address][0], address, len(tables[address])))

    return 0


if __name__ == "__main__":
    sys.exit(main())
