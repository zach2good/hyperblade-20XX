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

# Both images' .text, which holds every funclet a scope table names.
TEXT_START = 0x00401000
TEXT_END = 0x004B1000

# The opcode of `mov ecx, imm32`.
MOV_ECX = 0xB9

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "build" / "out"

#  0001:000059e0       ?Destroy@CDisplayPassBarPool@@UAEPAVAgendaItemOwner@@XZ 004069e0 f i cameraobject.cpp.obj
OBJECT_LINE = re.compile(r"^ 0001:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8}) f\s+(?:i\s+)?(\S+)$")


def read_map(path):
    """Answers each function the map lists, with its address: by name when the map lists the name
    once, by (name, object) when it lists the name in several objects."""
    return read_lines(path.read_text(errors="replace").splitlines())


def read_lines(lines):
    seen = collections.Counter()
    addresses = collections.defaultdict(set)
    for line in lines:
        found = OBJECT_LINE.match(line)
        if found:
            key = (found.group(1), found.group(3))
            seen[found.group(1)] += 1
            addresses[key].add(int(found.group(2), 16))

    # The linker relabels a folded static with the object of the copy it keeps, so one object can
    # list a name twice at one address.
    found = {}
    for (name, owner), addrs in addresses.items():
        if len(addrs) != 1:
            continue

        if seen[name] == 1:
            found[name] = min(addrs)
        else:
            found[(name, owner)] = min(addrs)

    return found


def relabelled(lines):
    """Answers each name a map lists twice in one object at one address: the linker gives a
    folded static the object of the copy it keeps."""
    seen = collections.Counter()
    addresses = collections.defaultdict(set)
    for line in lines:
        found = OBJECT_LINE.match(line)
        if found:
            key = (found.group(1), found.group(3))
            seen[key] += 1
            addresses[key].add(int(found.group(2), 16))

    return {name for (name, owner), count in seen.items()
            if count > 1 and len(addresses[(name, owner)]) == 1}


def fold_groups(measured, symbols, folded):
    """Answers the sets of HYPERX_sym.exe addresses that HYPERX.exe puts on one address. A copy
    of a name in `folded`, in an object HYPERX.map no longer lists, joins that name's address."""
    groups = collections.defaultdict(set)
    for name, addr in measured.items():
        if name in symbols:
            groups[addr].add(symbols[name])

    by_name = collections.defaultdict(set)
    for key, addr in measured.items():
        if isinstance(key, tuple):
            by_name[key[0]].add(addr)

    for key, addr in symbols.items():
        if isinstance(key, tuple) and key[0] in folded and key not in measured and (
                len(by_name[key[0]]) == 1):
            groups[min(by_name[key[0]])].add(addr)

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
    """Answers each function the map's "Static symbols" section lists, keyed as read_lines keys
    them."""
    lines = path.read_text(errors="replace").splitlines()
    start = next((number for number, line in enumerate(lines) if line.startswith(" Static symbols")),
                 len(lines))
    return read_lines(lines[start:])


def pair_map_symbols(db, rows, statics):
    """Pairs each row of symbols/map_pairs.csv with our static of that name in the map, in the
    row's object when it names one.

    Names on stderr each row whose symbol the map doesn't list, and answers how many it paired.
    """
    paired = 0
    with db.batch() as batch:
        for row in rows:
            key = row["symbol"]
            if row.get("object"):
                key = (row["symbol"], row["object"])

            ours = statics.get(key)
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


def in_text(addr):
    return TEXT_START <= addr < TEXT_END


def pair_scope_entries(db, orig_bin, recomp_bin, lookup, rows, sizes):
    """Pairs the filters and handlers of each paired `$scopetable` row in data_operands.csv.

    An entry is three words: the enclosing level, the filter (0 for a __finally) and the handler.
    The tables sit back to back with no terminator, so a walk stops at the first of these: the
    next row's table; a level that isn't -1 or an earlier entry; levels that differ between the
    two sides; a filter or handler outside .text on either side. A funclet takes its size from
    symbols/functions.csv and is skipped without a row there. Answers how many funclets it paired.
    """
    starts = sorted(int(row["address"], 16) for row in rows)
    paired = 0
    with db.batch() as batch:
        for row in rows:
            if not row["name"].endswith("$scopetable"):
                continue

            table = lookup(db, ImageId.ORIG, int(row["address"], 16))
            if table is None or table.recomp_addr is None:
                continue

            limit = next((start for start in starts if start > table.orig_addr),
                         table.orig_addr + 0x100)
            for entry in range((limit - table.orig_addr) // 12):
                theirs = orig_bin.read(table.orig_addr + entry * 12, 12)
                ours = recomp_bin.read(table.recomp_addr + entry * 12, 12)
                words = [(int.from_bytes(theirs[at:at + 4], "little"),
                          int.from_bytes(ours[at:at + 4], "little")) for at in (0, 4, 8)]
                (level, our_level), funclets = words[0], words[1:]
                if level != our_level or (level != 0xFFFFFFFF and level >= entry):
                    break

                if not all((orig == 0 and mine == 0) or (in_text(orig) and in_text(mine))
                           for orig, mine in funclets):
                    break

                for kind, (orig, mine) in zip(("filter", "handler"), funclets):
                    if orig == 0 or orig not in sizes:
                        continue

                    name = "%s$%s%d" % (row["name"][:-len("$scopetable")], kind, entry)
                    size = sizes[orig]
                    batch.set(ImageId.ORIG, orig, type=EntityType.FUNCTION, name=name, size=size)
                    batch.set(ImageId.RECOMP, mine, type=EntityType.FUNCTION, name=name, size=size)
                    batch.match(orig, mine)
                    paired += 1

    return paired


def function_sizes():
    """Answers each original function's size, as symbols/functions.csv gives it."""
    with open(ROOT / "symbols" / "functions.csv", encoding="utf-8") as handle:
        return {int(row["address"], 16): int(row["size"]) for row in csv.DictReader(handle)}


def install():
    """Patches reccmp so every Compare built afterwards reads the folding. Other tools call this."""
    measured = (OUT / "HYPERX.map").read_text(errors="replace").splitlines()
    groups = fold_groups(read_lines(measured), read_map(OUT / "HYPERX_sym.map"),
                         relabelled(measured))
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
        print("foldcmp: %d library funclet(s) paired through their scope tables"
              % pair_scope_entries(compare._db, compare.orig_bin, compare.recomp_bin, lookup,
                                   register("data_operands.csv"), function_sizes()),
              file=sys.stderr)
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
    """Checks, in turn:
    - map reading and grouping: three folded bodies, a name listed twice, a relabelled static;
    - statics through their thunks: one pairs, one thunk has no pair, one isn't a mov ecx;
    - the map's statics from "Static symbols" on: one listed, one missing, one by its object;
    - data through their functions: one pairs, one not held, one whose function has no pair;
    - scope tables: a __finally entry, a nested entry with a filter, a stop at the next table,
      and a stop where our level differs."""
    measured = (
        " 0001:0002a200       ?A@@UAEPAV1@XZ 0042b200 f i a.obj\n"
        " 0001:0002a200       ?B@@UAEPAV1@XZ 0042b200 f i a.obj\n"
        " 0001:0002a200       ?C@@UAEPAV1@XZ 0042b200 f i a.obj\n"
        " 0001:0002a300       ?D@@YAXXZ 0042b300 f   d.obj\n"
        " 0001:0002a400       _twice 0042b400 f   e.obj\n"
        " 0001:0002a500       _twice 0042b500 f   f.obj\n"
        " 0001:0002a600       _dup 0042b600 f   g.obj\n"
        " 0001:0002a600       _dup 0042b600 f   g.obj\n"
        " 0001:0002a800       _tri 0042b800 f   k.obj\n"
        " 0001:0002a800       _tri 0042b800 f   l.obj\n"
    )
    symbols = (
        " 0001:00001000       ?A@@UAEPAV1@XZ 00402000 f i a.obj\n"
        " 0001:00003000       ?B@@UAEPAV1@XZ 00404000 f i b.obj\n"
        " 0001:00005000       ?C@@UAEPAV1@XZ 00406000 f i c.obj\n"
        " 0001:00007000       ?D@@YAXXZ 00408000 f   d.obj\n"
        " 0001:00009000       _dup 0040a000 f   g.obj\n"
        " 0001:0000a000       _dup 0040b000 f   h.obj\n"
        " 0001:0000b000       _tri 0040c000 f   k.obj\n"
        " 0001:0000c000       _tri 0040d000 f   l.obj\n"
        " 0001:0000d000       _tri 0040e000 f   m.obj\n"
    )
    with tempfile.TemporaryDirectory() as folder:
        measured_path = Path(folder) / "measured.map"
        symbols_path = Path(folder) / "symbols.map"
        measured_path.write_text(measured)
        symbols_path.write_text(symbols)
        read = read_map(measured_path)
        groups = fold_groups(read, read_map(symbols_path), relabelled(measured.splitlines()))
        symbols_path.write_text(symbols + " Static symbols\n" + measured)
        statics = map_statics(symbols_path)

    assert "_twice" not in read
    # h.obj's _dup folded onto g.obj's, which the measured map lists twice. m.obj's _tri was
    # dropped, not relabelled, so it stays out of k.obj's and l.obj's group.
    expected = [{0x402000, 0x404000, 0x406000}, {0x40A000, 0x40B000}, {0x40C000, 0x40D000}]
    assert groups == expected, groups
    assert "?D@@YAXXZ" in statics and statics["?D@@YAXXZ"] == 0x42B300, statics
    assert "_twice" not in statics and statics[("_twice", "f.obj")] == 0x42B500, statics

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

    # A name in two objects, asked for by the second.
    symbols = [{"address": "0x004a0020", "symbol": "_twice", "size": "16", "object": "f.obj"}]
    statics = {("_twice", "e.obj"): 0x4B0400, ("_twice", "f.obj"): 0x4B0500}
    assert pair_map_symbols(db, symbols, statics) == 1
    assert db.get(ImageId.ORIG, 0x4A0020).recomp_addr == 0x4B0500

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

    # Two entries, then the next row's table, whose -1 entry the walk must not read as a third.
    def words(*values):
        return b"".join(value.to_bytes(4, "little") for value in values)

    theirs = words(0xFFFFFFFF, 0, 0x401100, 0, 0x401200, 0x401210, 0xFFFFFFFF, 0, 0x401300)
    ours = words(0xFFFFFFFF, 0, 0x402100, 0, 0x402200, 0x402210, 0xFFFFFFFF, 0, 0x402300)
    tables = [{"address": "0x004d0100", "name": "_f$scopetable", "function": "0x00410100"},
              {"address": "0x004d0118", "name": "_g$scopetable", "function": "0x00410200"}]
    with db.batch() as batch:
        batch.set(ImageId.ORIG, 0x4D0100, type=EntityType.DATA, name="_f$scopetable", size=4)
        batch.set(ImageId.RECOMP, 0x4E0100, type=EntityType.DATA, name="_f$scopetable", size=4)
        batch.match(0x4D0100, 0x4E0100)

    sizes = {0x401100: 8, 0x401200: 4, 0x401210: 8, 0x401300: 8}
    assert pair_scope_entries(db, Bytes(0x4D0100, theirs), Bytes(0x4E0100, ours), EntityDb.get,
                              tables, sizes) == 3
    assert db.get(ImageId.ORIG, 0x401200).recomp_addr == 0x402200
    assert db.get(ImageId.ORIG, 0x401300) is None

    # The same table, but our first entry sits at another level.
    ours = words(0, 0, 0x403100) + ours[12:]
    tables[0]["address"] = "0x004d0200"
    with db.batch() as batch:
        batch.set(ImageId.ORIG, 0x4D0200, type=EntityType.DATA, name="_f$scopetable", size=4)
        batch.set(ImageId.RECOMP, 0x4E0200, type=EntityType.DATA, name="_f$scopetable", size=4)
        batch.match(0x4D0200, 0x4E0200)

    tables[1]["address"] = "0x004d0218"
    assert pair_scope_entries(db, Bytes(0x4D0200, theirs), Bytes(0x4E0200, ours), EntityDb.get,
                              tables, sizes) == 0

    print("selftest passed")
    return 0


if __name__ == "__main__":
    if sys.argv[1:] == ["--selftest"]:
        raise SystemExit(selftest())

    raise SystemExit(main())
