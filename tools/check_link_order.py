"""Fail when a marked `$E` thunk pairs with another unit's thunk, or with the wrong thunk of its own.

VC6 names a unit's initialiser and atexit thunks `$E1`, `$E2` and so on, so several objects
define each name. reccmp pairs a by-name marker with the lowest unmatched function of that name,
both sides in address order, and our addresses follow the link order in `CMakeLists.txt`. So the
k-th marked `$En` in the client pairs with the k-th `$En` in our map, whichever object holds it.

`symbols/link_order.csv` names the object that owns each marked thunk, with the client's
evidence. The measured build's map says which objects define each name; `HYPERX_SOURCES` says the
order they link in. This fails on a marker whose partner lies in another object, and on a marker
of a shared name that the register does not list.

The right object can still hold the wrong thunk: a renumbered marker pairs with another `$En` of
the same file. So it also fingerprints each marked pair the way reccmp matches the startup table:
the matched entities each side's thunk references, in order, normalised to client addresses. A
destructor thunk names its destructor, an atexit registration the thunk it pushes, an initialiser
the data it writes. An unmatched address counts only as a placeholder, so two thunks that differ
only in unmatched data look alike.

    python tools/check_link_order.py          # exit 1 on a mispairing, 2 with no map
    python tools/check_link_order.py --list   # every marked name and its pairs
"""
import collections
import os
import re
import sys

import check_folded

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MAP = os.path.join(ROOT, "build", "out", "HYPERX_sym.map")
REGISTER = os.path.join(ROOT, "symbols", "link_order.csv")

MARKER = re.compile(r"^// SYNTHETIC: HYPERX (0x[0-9a-fA-F]+)\r?\n// (\$E\d+)\r?$", re.M)
MAP_ROW = re.compile(r"^\s*0001:[0-9a-f]{8}\s+_(\$E\d+)\s+([0-9a-f]{8}) f\s+(?:\S+:)?(\S+)\.obj\s*$")


def markers():
    found = collections.defaultdict(list)
    for where, _, names in os.walk(os.path.join(ROOT, "src")):
        for name in names:
            if name.endswith((".h", ".cpp")):
                text = open(os.path.join(where, name), encoding="utf-8", errors="replace").read()
                for address, symbol in MARKER.findall(text):
                    found[symbol].append(int(address, 16))
    return found


def candidates():
    found = collections.defaultdict(list)
    for line in open(MAP, encoding="utf-8", errors="replace"):
        m = MAP_ROW.match(line)
        if m:
            found[m.group(1)].append((int(m.group(2), 16), m.group(3)))
    return found


def link_order():
    text = open(os.path.join(ROOT, "CMakeLists.txt"), encoding="utf-8").read()
    block = text[text.index("set(HYPERX_SOURCES"):]
    block = block[:block.index(")")]
    return [os.path.basename(path) for path in re.findall(r"src/\S+\.cpp", block)]


def register():
    owners = {}
    for line in open(REGISTER, encoding="utf-8"):
        if line.startswith("0x"):
            fields = line.rstrip("\r\n").split(",", 2)
            owners[int(fields[0], 16)] = fields[1]
    return owners


ADDRESS = re.compile(r"0x[0-9a-f]{6,8}")


def fingerprint(compare, image, binary, address):
    """The client address of each matched entity the thunk references, or "?" for any other."""
    from reccmp.compare.asm.instgen import InstructGen, SectionType
    from reccmp.types import ImageId

    db = compare._db
    size = db.get(image, address).size(image) or 32
    found = []

    for section in InstructGen(bytes(binary.read(address, size)), address, True).sections:
        if section.type != SectionType.CODE:
            continue

        for instruction in section.contents:
            mnemonic, operands = instruction[2:]

            for text in ADDRESS.findall(operands):
                entity = db.get(image, int(text, 16), exact=True)
                found.append("0x%08x" % entity.addr(ImageId.ORIG) if entity and entity.matched else "?")

            if mnemonic in ("ret", "jmp"):
                return found

    return found


def mispaired_thunks(marked):
    """Each marked thunk whose partner references other entities than the client's thunk."""
    import logging

    from reccmp.types import ImageId

    logging.disable(logging.CRITICAL)
    compare = check_folded.engine()
    faults = []

    for symbol in sorted(marked, key=lambda s: int(s[2:])):
        for address in sorted(marked[symbol]):
            entity = compare._db.get(ImageId.ORIG, address, exact=True)
            partner = entity.addr(ImageId.RECOMP) if entity else None

            if partner is None:
                faults.append("%s 0x%08x pairs with nothing" % (symbol, address))
                continue

            theirs = fingerprint(compare, ImageId.ORIG, compare.orig_bin, address)
            mine = fingerprint(compare, ImageId.RECOMP, compare.recomp_bin, partner)

            if theirs != mine:
                faults.append("%s 0x%08x references %s and its partner 0x%08x %s"
                              % (symbol, address, " ".join(theirs), partner, " ".join(mine)))

    return faults


def main():
    if not os.path.exists(MAP):
        print("no %s; build first" % os.path.relpath(MAP, ROOT))
        return 2
    show = "--list" in sys.argv
    marked = markers()
    ours = candidates()
    owners = register()
    position = dict((name, i) for i, name in enumerate(link_order()))
    faults = []
    for symbol in sorted(marked, key=lambda s: int(s[2:])):
        theirs = sorted(marked[symbol])
        mine = sorted(ours.get(symbol, []), key=lambda row: (position.get(row[1], len(position)), row[0]))
        if len(mine) < 2:
            continue
        for k, address in enumerate(theirs):
            want = owners.get(address)
            got = mine[k][1] if k < len(mine) else None
            if show:
                print("%-5s 0x%08x  wants %-32s pairs %s" % (symbol, address, want, got))
            if want is None:
                faults.append("%s 0x%08x has no row in symbols/link_order.csv" % (symbol, address))
            elif want != got:
                faults.append("%s 0x%08x belongs to %s and pairs with %s's" % (symbol, address, want, got))
    faults += mispaired_thunks(marked)
    for fault in faults:
        print("MISPAIRED " + fault)
    print("%d marked thunk name(s), %d mispairing(s)" % (len(marked), len(faults)))
    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
