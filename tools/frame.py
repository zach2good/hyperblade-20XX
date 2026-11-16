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
    python tools/frame.py 0x004657b0 --calls     # both, the slots each call takes, side by side
    python tools/frame.py 0x004657b0 --listing   # every instruction with its depth
    python tools/frame.py --selftest

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
EH_FRAME = ("-1", "0xffffffff")
# The saved registers that end the stretch where a `push ecx` reserves four bytes.
REGISTERS = ("ebx", "esi", "edi")




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
    """ESP relative to the entry, once the prologue's register pushes are done. A C++ EH frame's
    `push -1`, then its `push <handler>`, and the `push eax` of the old fs:[0] count as prologue
    too; outside such a frame, an immediate or EAX pushed early is a call's argument. The frame
    opens the function, after `push ebp` at most, with the handler push straight after `push -1`.
    A `push ecx` before EBX, ESI or EDI reserves four bytes, unless ECX was written first, which
    makes it an argument."""
    depth = 0
    framed = False
    handler = False
    linked = False
    written = False
    pushes = []
    for position, instruction in enumerate(code):
        mnemonic = instruction.mnemonic
        operands = instruction.op_str
        if mnemonic == "mov" and operands.endswith("fs:[0]"):
            linked = True
            continue

        if mnemonic == "sub" and operands.startswith("esp,"):
            depth -= int(operands.split(",")[1], 0)
            continue

        if mnemonic in ("mov", "lea", "xor", "test", "cmp"):
            written = written or (mnemonic != "test" and mnemonic != "cmp"
                                  and operands.startswith("ecx,"))
            continue

        if mnemonic != "push":
            break

        following = code[position + 1] if position + 1 < len(code) else None
        if (operands in EH_FRAME and pushes in ([], ["ebp"]) and following is not None
                and following.mnemonic == "push" and following.op_str.startswith("0x")):
            framed = True
        elif framed and not handler and operands.startswith("0x"):
            handler = True
        elif linked and operands == "eax":
            linked = False
        elif operands == "ecx" and not written and not [saved for saved in pushes
                                                         if saved in REGISTERS]:
            pass
        elif operands not in SAVED:
            break

        pushes.append(operands)
        depth -= 4

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


def calls(listing):
    """Each call's target and the slots the `lea`s since the previous call took the address of."""
    found = []
    taken = []
    for at, text, offsets, depth in listing:
        if text.startswith("lea "):
            taken.extend(offsets)
        elif text.startswith("call "):
            found.append((text[len("call "):], tuple(taken)))
            taken = []
    return found


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
    parser.add_argument("--calls", action="store_true")
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
    if not options.ours or options.calls or options.compare:
        code = disassemble(reference_path(), address, size)
        sides.append(("client 0x%08x" % address, code))
    if options.ours or options.compare or options.calls:
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
        maps.append((title, slots, listing))
        if options.calls:
            continue
        if options.listing:
            print(title)
            for at, text, offsets, depth in listing:
                tag = " ".join("{%+05x}" % o for o in offsets)
                print("  %08x  d=%-6d %-46s %s" % (at, depth, text, tag))
        else:
            show(slots, title)

    if options.calls and len(maps) == 2:
        # Paired by call order; a swap of roles inside an equal frame shows up here.
        # Both targets print, so a call one side inlines shows as the pairing shifting.
        row = "%-2s %-4s %-20s %-26s %-20s %s"
        print(row % ("", "", "client call", "slots", "our call", "slots"))
        theirs = calls(maps[0][2])
        ours = calls(maps[1][2])
        for number in range(max(len(theirs), len(ours))):
            left = theirs[number] if number < len(theirs) else ("-", ())
            right = ours[number] if number < len(ours) else ("-", ())
            mark = "  " if left[1] == right[1] else "!="
            print(row % (mark, number, left[0][:20], " ".join("%+05x" % o for o in left[1]),
                         right[0][:20], " ".join("%+05x" % o for o in right[1])))

    if options.compare and len(maps) == 2:
        left = maps[0][1]
        right = maps[1][1]
        print("slots only one side has")
        for offset in sorted(set(left) ^ set(right)):
            side = "client" if offset in left else "ours"
            source = left if offset in left else right
            print("  %+05x  %-6s %s" % (offset, side, source[offset][0][1][:56]))
    return 0


def selftest():
    """Checks that an EH prologue settles past its three pushes and a plain one is unchanged."""
    class Instruction:
        def __init__(self, mnemonic, op_str):
            self.mnemonic = mnemonic
            self.op_str = op_str

    plain = [Instruction("push", "ebx"), Instruction("sub", "esp, 0x10"),
             Instruction("call", "0x401000")]
    assert settled(plain) == -0x14, settled(plain)
    argument = [Instruction("push", "esi"), Instruction("push", "0x68"), Instruction("push", "eax"),
                Instruction("call", "0x401000")]
    assert settled(argument) == -4, settled(argument)
    # An EAX pushed without the fs:[0] load is an argument, and so is a -1 after a saved push.
    unlinked = [Instruction("push", "esi"), Instruction("push", "eax"),
                Instruction("call", "0x401000")]
    assert settled(unlinked) == -4, settled(unlinked)
    late = [Instruction("push", "esi"), Instruction("push", "-1"), Instruction("call", "0x401000")]
    assert settled(late) == -4, settled(late)
    reserve = [Instruction("push", "ecx"), Instruction("push", "ebx"),
               Instruction("call", "0x401000")]
    assert settled(reserve) == -8, settled(reserve)
    # A -1 with no handler after it is an argument; a reserve after the EH link still counts.
    minus = [Instruction("push", "-1"), Instruction("push", "edx"), Instruction("call", "0x401000")]
    assert settled(minus) == 0, settled(minus)
    linked = [Instruction("push", "-1"), Instruction("push", "0x4ac990"),
              Instruction("mov", "eax, dword ptr fs:[0]"), Instruction("push", "eax"),
              Instruction("mov", "dword ptr fs:[0], esp"), Instruction("push", "ecx"),
              Instruction("push", "esi"), Instruction("call", "0x401000")]
    assert settled(linked) == -20, settled(linked)
    # GDI::BeginPaint's shape: ECX loaded, then pushed as an argument.
    loaded = [Instruction("mov", "ecx, dword ptr [esp + 4]"), Instruction("push", "ecx"),
              Instruction("push", "eax"), Instruction("call", "0x401000")]
    assert settled(loaded) == 0, settled(loaded)
    framed = [Instruction("push", "ebp"), Instruction("mov", "ebp, esp"), Instruction("push", "-1"),
              Instruction("push", "0x4ac990"), Instruction("mov", "eax, dword ptr fs:[0]"),
              Instruction("push", "eax"), Instruction("mov", "dword ptr fs:[0], esp"),
              Instruction("sub", "esp, 0x1a8"), Instruction("call", "0x401000")]
    assert settled(framed) == -(0x10 + 0x1a8), settled(framed)
    # One call each side: the client passes two slots, ours one, and a lea off a register
    # adds none.
    theirs = calls([(0, "lea eax, [esp + 0x10]", [-0x20], -0x30), (1, "push eax", [], -0x30),
                    (2, "lea edx, [eax + 4]", [], -0x34),
                    (3, "lea ecx, [esp + 0x18]", [-0x14], -0x34), (4, "call 0x401000", [], -0x34)])
    ours = calls([(0, "lea eax, [esp + 0x14]", [-0x1c], -0x30), (1, "push eax", [], -0x30),
                  (2, "call 0x401000", [], -0x34)])
    assert theirs == [("0x401000", (-0x20, -0x14))], theirs
    assert ours == [("0x401000", (-0x1c,))], ours
    print("selftest passed")
    return 0


if __name__ == "__main__":
    if sys.argv[1:] == ["--selftest"]:
        sys.exit(selftest())

    sys.exit(main())
