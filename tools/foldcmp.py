"""Run reccmp's asmcmp with the measured build's identical-code folding applied.

reccmp reads HYPERX_sym.exe, which links /OPT:NOREF and so keeps every identical body apart.
HYPERX.exe links /OPT:REF, which in VC6 folds them, as the original's link did. Where HYPERX.map
puts several functions at one address and HYPERX_sym.map keeps them apart, every recompiled
reference to a copy without a marker reads as a reference to the one copy with a marker. Call
operands and vtable slots then name what HYPERX.exe really calls.

Takes asmcmp's arguments unchanged:

    python tools/foldcmp.py --target HYPERX --json build/report.json --silent
    python tools/foldcmp.py --target HYPERX --verbose 0x004b3794 --no-color
    python tools/foldcmp.py --selftest
"""

import collections
import csv
import re
import sys
import tempfile
from pathlib import Path

from reccmp.compare import functions
from reccmp.compare.db import EntityDb
from reccmp.tools import asmcmp
from reccmp.types import EntityType, ImageId

# The opcode of `mov ecx, imm32`.
MOV_ECX = 0xB9

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "build" / "out"

# 0001:000059e0       ?Destroy@CDisplayPassBarPool@@UAEPAVAgendaItemOwner@@XZ 004069e0 f i cameraobject.cpp.obj
FUNCTION_LINE = re.compile(r"^ 0001:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8}) f ")


def read_map(path):
    """Answers each function name the map lists once, with its address."""
    return read_lines(path.read_text(errors="replace").splitlines())


def read_lines(lines):
    seen = collections.Counter()
    addresses = {}
    for line in lines:
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
    """Answers, for each unmarked copy in a group with exactly one marked copy, that copy.

    Names on stderr each group it skips for holding several marked copies, which no alias can
    settle: the original keeps those bodies apart, so this build folds what the original doesn't.
    """
    aliases = {}
    for group in groups:
        marked = []
        for addr in group:
            entity = lookup(db, ImageId.RECOMP, addr)
            if entity is not None and entity.orig_addr is not None:
                marked.append(addr)

        if len(marked) > 1:
            names = sorted(lookup(db, ImageId.RECOMP, addr).name for addr in marked)
            print("foldcmp: one body holds %d marked copies: %s" % (len(marked), ", ".join(names)),
                  file=sys.stderr)

        if len(marked) != 1:
            continue

        for addr in group:
            if addr != marked[0]:
                aliases[addr] = marked[0]

    return aliases


def register(name):
    lines = (line for line in open(ROOT / "symbols" / name, encoding="utf-8") if not line.startswith("#"))
    return list(csv.DictReader(lines))


def pair_statics(db, recomp_bin, lookup, rows):
    """Pairs each row, as symbols/static_objects.csv gives it, with our copy, read off its thunk.

    Our PDB has no symbol for a function-scope static. Its atexit thunk, which pairs by name,
    starts `mov ecx, <object>` on both sides, so our object's address is the thunk's immediate.
    Names on stderr each row it can't pair, and answers how many it paired.
    """
    paired = 0
    with db.batch() as batch:
        for row in rows:
            thunk = lookup(db, ImageId.ORIG, int(row["thunk"], 16))
            if thunk is None or thunk.recomp_addr is None:
                print("foldcmp: %s: its thunk %s has no pair" % (row["name"], row["thunk"]), file=sys.stderr)
                continue

            code = recomp_bin.read(thunk.recomp_addr, 5)
            if code[0] != MOV_ECX:
                print("foldcmp: %s: our thunk does not start with mov ecx" % row["name"], file=sys.stderr)
                continue

            ours = int.from_bytes(code[1:5], "little")
            size = int(row["size"], 16)
            orig = int(row["address"], 16)
            batch.set(ImageId.ORIG, orig, type=EntityType.DATA, name=row["name"], size=size)
            batch.set(ImageId.RECOMP, ours, type=EntityType.DATA, name=row["name"], size=size)
            batch.match(orig, ours)
            paired += 1

    return paired


def map_statics(path):
    """Answers each function the map's "Static symbols" section lists once, with its address."""
    lines = path.read_text(errors="replace").splitlines()
    start = next((number for number, line in enumerate(lines) if line.startswith(" Static symbols")),
                 len(lines))
    return read_lines(lines[start:])


def pair_map_symbols(db, rows, statics):
    """Pairs each row of symbols/map_pairs.csv with our static of that name in the map.

    Names on stderr each row whose symbol the map doesn't list, and answers how many it paired.
    """
    paired = 0
    with db.batch() as batch:
        for row in rows:
            ours = statics.get(row["symbol"])
            if ours is None:
                print("foldcmp: %s is not a static in the map" % row["symbol"], file=sys.stderr)
                continue

            orig = int(row["address"], 16)
            size = int(row["size"])
            batch.set(ImageId.ORIG, orig, type=EntityType.FUNCTION, name=row["symbol"], size=size)
            batch.set(ImageId.RECOMP, ours, type=EntityType.FUNCTION, name=row["symbol"], size=size)
            batch.match(orig, ours)
            paired += 1

    return paired


def pair_operands(db, orig_bin, recomp_bin, lookup, rows):
    """Pairs each row of symbols/data_operands.csv with the datum our copy of its function names.

    The row's address is an immediate in the original's function. Ours sits at the same offset
    in our copy. Names on stderr each row it can't pair, and answers how many it paired.
    """
    paired = 0
    with db.batch() as batch:
        for row in rows:
            function = lookup(db, ImageId.ORIG, int(row["function"], 16))
            if function is None or function.recomp_addr is None:
                print("foldcmp: %s: its function %s has no pair" % (row["name"], row["function"]),
                      file=sys.stderr)
                continue

            orig = int(row["address"], 16)
            size = function.any_size(ImageId.ORIG)
            offset = orig_bin.read(function.orig_addr, size).find(orig.to_bytes(4, "little"))
            if offset < 0:
                print("foldcmp: %s: %s doesn't hold its address" % (row["name"], row["function"]),
                      file=sys.stderr)
                continue

            ours = int.from_bytes(recomp_bin.read(function.recomp_addr + offset, 4), "little")
            batch.set(ImageId.ORIG, orig, type=EntityType.DATA, name=row["name"], size=4)
            batch.set(ImageId.RECOMP, ours, type=EntityType.DATA, name=row["name"], size=4)
            batch.match(orig, ours)
            paired += 1

    return paired


def install():
    """Patches reccmp so every Compare built afterwards reads the folding. Other tools call this."""
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
        print("foldcmp: %d function-scope static(s) paired through their thunks"
              % pair_statics(compare._db, compare.recomp_bin, lookup, register("static_objects.csv")),
              file=sys.stderr)
        statics = map_statics(OUT / "HYPERX_sym.map")
        print("foldcmp: %d library static(s) paired by their map names"
              % pair_map_symbols(compare._db, register("map_pairs.csv"), statics), file=sys.stderr)
        print("foldcmp: %d library datum(s) paired through their functions"
              % pair_operands(compare._db, compare.orig_bin, compare.recomp_bin, lookup,
                              register("data_operands.csv")), file=sys.stderr)
        aliases.update(aliases_for(compare._db, groups, lookup))
        print("foldcmp: %d fold groups, %d copies read as their marked twin"
              % (len(groups), len(aliases)), file=sys.stderr)
        return compare

    EntityDb.get = get
    functions.create_name_lookup = create_name_lookup
    asmcmp.Compare.from_target = classmethod(from_target)


def main():
    install()
    return asmcmp.main()


def selftest():
    """Checks the map reading and grouping on three folded bodies and one name listed twice; the
    pairing of statics through their thunks: one pairs, one thunk has no pair, one isn't a mov
    ecx; the map's statics, read from "Static symbols" on: one listed, one missing; and the
    pairing of data through their functions: one pairs, one the function doesn't hold, one whose
    function has no pair."""
    measured = (
        " 0001:0002a200       ?A@@UAEPAV1@XZ 0042b200 f i a.obj\n"
        " 0001:0002a200       ?B@@UAEPAV1@XZ 0042b200 f i a.obj\n"
        " 0001:0002a200       ?C@@UAEPAV1@XZ 0042b200 f i a.obj\n"
        " 0001:0002a300       ?D@@YAXXZ 0042b300 f   d.obj\n"
        " 0001:0002a400       _twice 0042b400 f   e.obj\n"
        " 0001:0002a500       _twice 0042b500 f   f.obj\n"
    )
    symbols = (
        " 0001:00001000       ?A@@UAEPAV1@XZ 00402000 f i a.obj\n"
        " 0001:00003000       ?B@@UAEPAV1@XZ 00404000 f i b.obj\n"
        " 0001:00005000       ?C@@UAEPAV1@XZ 00406000 f i c.obj\n"
        " 0001:00007000       ?D@@YAXXZ 00408000 f   d.obj\n"
    )
    with tempfile.TemporaryDirectory() as folder:
        measured_path = Path(folder) / "measured.map"
        symbols_path = Path(folder) / "symbols.map"
        measured_path.write_text(measured)
        symbols_path.write_text(symbols)
        read = read_map(measured_path)
        groups = fold_groups(read, read_map(symbols_path))
        symbols_path.write_text(symbols + " Static symbols\n" + measured)
        statics = map_statics(symbols_path)

    assert "_twice" not in read
    assert groups == [{0x402000, 0x404000, 0x406000}], groups
    assert "?D@@YAXXZ" in statics and statics["?D@@YAXXZ"] == 0x42B300, statics

    # Two paired thunks; ours for the first starts mov ecx, 0x00499000, the second doesn't.
    class Image:
        def read(self, addr, size):
            return {0x420000: bytes([MOV_ECX, 0x00, 0x90, 0x49, 0x00]),
                    0x420010: bytes([0x90] * 5)}[addr][:size]

    db = EntityDb()
    with db.batch() as batch:
        for orig, ours in ((0x410000, 0x420000), (0x410010, 0x420010)):
            batch.set(ImageId.ORIG, orig, type=EntityType.FUNCTION, name="thunk")
            batch.set(ImageId.RECOMP, ours, type=EntityType.FUNCTION, name="thunk")
            batch.match(orig, ours)

    rows = [{"address": "0x004c0000", "name": "A::s_one", "size": "0x7c", "thunk": "0x00410000"},
            {"address": "0x004c0100", "name": "B::s_two", "size": "0x7c", "thunk": "0x00410010"},
            {"address": "0x004c0200", "name": "C::s_three", "size": "0x7c", "thunk": "0x00410020"}]
    assert pair_statics(db, Image(), EntityDb.get, rows) == 1
    assert db.get(ImageId.ORIG, 0x4C0000).recomp_addr == 0x499000

    # One library static the map lists, one it doesn't.
    symbols = [{"address": "0x004a0000", "symbol": "_listed", "size": "16"},
               {"address": "0x004a0010", "symbol": "_missing", "size": "16"}]
    assert pair_map_symbols(db, symbols, {"_listed": 0x4B0000}) == 1
    assert db.get(ImageId.ORIG, 0x4A0000).recomp_addr == 0x4B0000

    # A paired function that pushes 0x004d0000; ours pushes 0x004e0000 at the same offset.
    class Bytes:
        def __init__(self, at, code):
            self.at = at
            self.code = code

        def read(self, addr, size):
            return self.code[addr - self.at:addr - self.at + size]

    with db.batch() as batch:
        batch.set(ImageId.ORIG, 0x410100, type=EntityType.FUNCTION, name="pusher", size=8)
        batch.set(ImageId.RECOMP, 0x420100, type=EntityType.FUNCTION, name="pusher", size=8)
        batch.match(0x410100, 0x420100)

    data = [{"address": "0x004d0000", "name": "_scope", "function": "0x00410100"},
            {"address": "0x004d0010", "name": "_absent", "function": "0x00410100"},
            {"address": "0x004d0020", "name": "_lonely", "function": "0x00410200"}]
    orig_bin = Bytes(0x410100, bytes([0x90, 0x68, 0x00, 0x00, 0x4D, 0x00, 0x90, 0x90]))
    recomp_bin = Bytes(0x420100, bytes([0x90, 0x68, 0x00, 0x00, 0x4E, 0x00, 0x90, 0x90]))
    assert pair_operands(db, orig_bin, recomp_bin, EntityDb.get, data) == 1
    assert db.get(ImageId.ORIG, 0x4D0000).recomp_addr == 0x4E0000

    print("selftest passed")
    return 0


if __name__ == "__main__":
    if sys.argv[1:] == ["--selftest"]:
        raise SystemExit(selftest())

    raise SystemExit(main())
