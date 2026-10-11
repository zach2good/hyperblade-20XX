"""Canonical frame slots for one function, on either side, as a map you can diff.

An `[ESP + N]` displacement is not a frame slot. A `PUSH` before a call shifts every
displacement after it, and VC6 pushes `EBX`/`EBP` mid function rather than in the prologue.
This tracks the depth through every push, pop, `ADD ESP` and both call-cleanup conventions,
and prints each reference relative to the entry ESP. Measuring from the settled ESP after the
prologue would show a false four-byte shift wherever one side pushes a register mid body.

The census stops at the prologue's own reservation, because a `__stdcall` callee that pops its
own arguments leaves nothing in the listing to say so and the depth model drifts above it. Pass
`--limit` to read further, and treat the result as a lead.

The report's hunks elide instructions, so this disassembles the whole function from
`reccmp-user.yml`'s original and `build/out/HYPERX_sym.exe`.

    python tools/frame.py 0x004657b0             # the client's census
    python tools/frame.py 0x004657b0 --ours      # this build's, at the paired address
    python tools/frame.py 0x004657b0 --compare   # both, and the slots only one side has
    python tools/frame.py 0x004657b0 --listing   # every instruction with its depth

Exit 1 when the address is not a row in `symbols/functions.csv` or the report has no pairing
for it, and 2 when there is nothing built to read.
"""

import argparse
import collections
import csv
import json
import os
import re
import sys

import capstone
import pefile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from reference import reference_path

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPORT = os.path.join(ROOT, "build", "report.json")
OURS = os.path.join(ROOT, "build", "out", "HYPERX_sym.exe")
FUNCTIONS = os.path.join(ROOT, "symbols", "functions.csv")

DISPLACEMENT = re.compile(r"\[esp \+ (0x[0-9a-f]+)\]")
BARE = re.compile(r"\[esp\]")
SAVED = ("ebx", "ebp", "esi", "edi")




def disassemble(path, address, size):
    image = pefile.PE(path, fast_load=True)
    data = image.get_memory_mapped_image()
    start = address - image.OPTIONAL_HEADER.ImageBase
    engine = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    return list(engine.disasm(data[start:start + size], address))


def reserved(code):
    """What the prologue set aside. Above it the depth model has usually drifted."""
    for instruction in code:
        if instruction.mnemonic == "sub" and instruction.op_str.startswith("esp,"):
            return int(instruction.op_str.split(",")[1], 0)
    return 0


def settled(code):
    """ESP relative to the entry, once the prologue's register pushes are done."""
    depth = 0
    for instruction in code:
        if instruction.mnemonic == "push" and instruction.op_str in SAVED:
            depth -= 4
            continue
        if instruction.mnemonic == "sub" and instruction.op_str.startswith("esp,"):
            depth -= int(instruction.op_str.split(",")[1], 0)
            continue
        if instruction.mnemonic in ("mov", "lea", "xor", "test", "cmp"):
            continue
        break
    return depth


def census(code, base):
    """Every canonical slot the body touches, and what touched it."""
    slots = collections.defaultdict(list)
    listing = []
    depth = 0
    for position, instruction in enumerate(code):
        text = "%s %s" % (instruction.mnemonic, instruction.op_str)
        offsets = [int(m, 0) + depth for m in DISPLACEMENT.findall(text)]
        if BARE.search(text):
            offsets.append(depth)
        for offset in offsets:
            slots[offset].append((instruction.address, text))
        listing.append((instruction.address, text, offsets, depth))
        depth = advance(code, position, depth, base)
    return slots, listing


def advance(code, position, depth, base):
    """The stack effect of one instruction, both cleanup conventions included."""
    instruction = code[position]
    if instruction.mnemonic == "push":
        return depth - 4
    if instruction.mnemonic == "pop":
        return depth + 4
    if instruction.mnemonic in ("add", "sub") and instruction.op_str.startswith("esp,"):
        step = int(instruction.op_str.split(",")[1], 0)
        if instruction.mnemonic == "add":
            return depth + step
        return depth - step
    if instruction.mnemonic == "call":
        # A __stdcall callee pops its own arguments and leaves nothing to read here, so
        # the depth is back at the frame; a cdecl one announces itself with the ADD.
        for ahead in code[position + 1:position + 4]:
            if ahead.mnemonic == "call":
                break
            if ahead.mnemonic == "add" and ahead.op_str.startswith("esp,"):
                return depth
        return base
    return depth


def paired(address):
    """Where this build put the same function, read from the report."""
    with open(REPORT, encoding="utf-8") as handle:
        report = json.load(handle)
    for entry in report.get("data", []):
        if int(entry["address"], 16) == address:
            return int(entry["recomp"], 16), entry["name"], entry["matching"]
    return None, None, None


def size_of(address):
    with open(FUNCTIONS, encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            if int(row["address"], 16) == address:
                return int(row["size"])
    return None


def show(slots, title):
    print(title)
    for offset in sorted(slots):
        print("  %+05x  n=%-3d  %s" % (offset, len(slots[offset]), slots[offset][0][1][:56]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("address")
    parser.add_argument("--ours", action="store_true")
    parser.add_argument("--compare", action="store_true")
    parser.add_argument("--listing", action="store_true")
    parser.add_argument("--limit", type=lambda v: int(v, 0), default=None,
                        help="highest canonical offset to report; defaults to the "
                             "prologue's own reservation, above which the depth model "
                             "has usually drifted")
    options = parser.parse_args()

    if not os.path.exists(REPORT) or not os.path.exists(OURS):
        print("no build/report.json or build/out/HYPERX_sym.exe; build and rescore first")
        return 2

    address = int(options.address, 16)
    size = size_of(address)
    if size is None:
        print("0x%08x is not a row in symbols/functions.csv" % address)
        return 1

    recomp, name, score = paired(address)
    if recomp is None:
        print("0x%08x has no pairing in build/report.json" % address)
        return 1

    reservation = options.limit
    sides = []
    if not options.ours:
        code = disassemble(reference_path(), address, size)
        sides.append(("client 0x%08x" % address, code))
    if options.ours or options.compare:
        code = disassemble(OURS, recomp, size)
        sides.append(("ours   0x%08x" % recomp, code))

    print("%s  %.2f%%  %d bytes" % (name, score * 100.0, size))
    maps = []
    for title, code in sides:
        base = settled(code)
        slots, listing = census(code, base)
        if reservation is None:
            reservation = reserved(code)
        slots = {k: v for k, v in slots.items() if base <= k < 0}
        maps.append((title, slots))
        if options.listing:
            print(title)
            for at, text, offsets, depth in listing:
                tag = " ".join("{%+05x}" % o for o in offsets)
                print("  %08x  d=%-6d %-46s %s" % (at, depth, text, tag))
        else:
            show(slots, title)

    if options.compare and len(maps) == 2:
        left = maps[0][1]
        right = maps[1][1]
        print("slots only one side has")
        for offset in sorted(set(left) ^ set(right)):
            side = "client" if offset in left else "ours"
            source = left if offset in left else right
            print("  %+05x  %-6s %s" % (offset, side, source[offset][0][1][:56]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
