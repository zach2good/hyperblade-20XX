"""Run reccmp's asmcmp with the measured build's identical-code folding applied.

reccmp reads HYPERX_sym.exe, which links /OPT:NOREF and so keeps every identical body apart.
HYPERX.exe links /OPT:REF, which in VC6 folds them, as the original's link did. Where HYPERX.map
puts several functions at one address and HYPERX_sym.map keeps them apart, every recompiled
reference to a copy without a marker reads as a reference to the one copy with a marker. Call
operands and vtable slots then name what HYPERX.exe really calls.

Takes asmcmp's arguments unchanged:

    python tools/foldcmp.py --target HYPERX --json build/report.json --silent
    python tools/foldcmp.py --target HYPERX --verbose 0x004b3794 --no-color
"""

import collections
import re
import sys
from pathlib import Path

from reccmp.compare import functions
from reccmp.compare.db import EntityDb
from reccmp.tools import asmcmp
from reccmp.types import ImageId

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "build" / "out"

# 0001:0002a200       ?Destroy@NodeOwner@@UAEPAV1@XZ 0042b200 f i planvm.cpp.obj
FUNCTION_LINE = re.compile(r"^ 0001:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8}) f ")


def read_map(path):
    """Answers each function name the map lists once, with its address."""
    seen = collections.Counter()
    addresses = {}
    for line in path.read_text(errors="replace").splitlines():
        found = FUNCTION_LINE.match(line)
        if found:
            seen[found.group(1)] += 1
            addresses[found.group(1)] = int(found.group(2), 16)

    return {name: addr for name, addr in addresses.items() if seen[name] == 1}


def fold_groups(measured, symbols):
    """Answers the sets of HYPERX_sym.exe addresses that HYPERX.exe puts on one address."""
    groups = collections.defaultdict(set)
    for name, addr in measured.items():
        if name in symbols:
            groups[addr].add(symbols[name])

    return [group for group in groups.values() if len(group) > 1]


def aliases_for(db, groups, lookup):
    """Answers, for each unmarked copy in a group with exactly one marked copy, that copy."""
    aliases = {}
    for group in groups:
        marked = []
        for addr in group:
            entity = lookup(db, ImageId.RECOMP, addr)
            if entity is not None and entity.orig_addr is not None:
                marked.append(addr)

        if len(marked) != 1:
            continue

        for addr in group:
            if addr != marked[0]:
                aliases[addr] = marked[0]

    return aliases


def main():
    groups = fold_groups(read_map(OUT / "HYPERX.map"), read_map(OUT / "HYPERX_sym.map"))
    lookup = EntityDb.get
    aliases = {}

    def get(self, img, addr, *, exact=True):
        if img == ImageId.RECOMP:
            addr = aliases.get(addr, addr)

        return lookup(self, img, addr, exact=exact)

    # The name lookup measures an operand's offset from the address it was asked about, so a
    # call to an unmarked copy has to be asked about as the marked copy.
    make_lookup = functions.create_name_lookup

    def create_name_lookup(db, image_id, *args, **kwargs):
        name_of = make_lookup(db, image_id, *args, **kwargs)
        if image_id != ImageId.RECOMP:
            return name_of

        def lookup(addr, exact=False, indirect=False):
            return name_of(aliases.get(addr, addr), exact, indirect)

        return lookup

    build = asmcmp.Compare.from_target.__func__

    def from_target(cls, target):
        compare = build(cls, target)
        aliases.update(aliases_for(compare._db, groups, lookup))
        print("foldcmp: %d fold groups, %d copies read as their marked twin"
              % (len(groups), len(aliases)), file=sys.stderr)
        return compare

    EntityDb.get = get
    functions.create_name_lookup = create_name_lookup
    asmcmp.Compare.from_target = classmethod(from_target)
    return asmcmp.main()


if __name__ == "__main__":
    raise SystemExit(main())
