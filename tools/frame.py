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
# String instructions, whose operands name memory rather than the registers they write.
STRING_OPS = ("movsd", "stosd", "movsb", "stosb")
# Registers an instruction writes without naming them as its first operand.
IMPLICIT = {"cdq": ("edx",), "mul": ("eax", "edx"), "imul": ("eax", "edx"), "div": ("eax", "edx"),
            "idiv": ("eax", "edx"), "rep movsd": ("esi", "edi", "ecx"),
            "rep stosd": ("edi", "ecx"), "rep movsb": ("esi", "edi", "ecx"),
            "rep stosb": ("edi", "ecx"), "movsd": ("esi", "edi"), "stosd": ("edi",),
            "movsb": ("esi", "edi"), "stosb": ("edi",), "repne scasb": ("edi", "ecx")}
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
    frame = base
    saves = []
    arrivals = {}
    for position, instruction in enumerate(code):
        # Code after a jmp or ret starts at the depth of the first jump that reaches it.
        if (position and code[position - 1].mnemonic in ("jmp", "ret")
                and instruction.address in arrivals):
            depth, frame, saves = arrivals[instruction.address]
            saves = list(saves)
        text = "%s %s" % (instruction.mnemonic, instruction.op_str)
        if instruction.mnemonic.startswith("j") and instruction.op_str.startswith("0x"):
            arrivals.setdefault(int(instruction.op_str, 16), (depth, frame, tuple(saves)))
        offsets = [int(m, 0) + depth for m in DISPLACEMENT.findall(text)]
        if BARE.search(text):
            offsets.append(depth)
        for offset in offsets:
            slots[offset].append((instruction.address, text))
        listing.append((instruction.address, text, offsets, depth))
        register = instruction.op_str
        if instruction.mnemonic == "push" and register in SAVED and depth == frame:
            if saves_register(code, position, register):
                frame -= 4
                saves.append(register)
        elif instruction.mnemonic == "pop" and register in saves:
            saves.remove(register)
            frame += 4
        depth = advance(code, position, depth, frame)
    return slots, listing


def saves_register(code, position, register):
    """Whether a mid-body push of a callee-saved register is a save rather than an argument: the
    register is written before it is read, before the next call, push or jump."""
    for ahead in code[position + 1:position + 13]:
        if ahead.mnemonic == "push" and ahead.op_str in SAVED:
            continue
        if ahead.mnemonic in ("call", "push", "ret", "jmp"):
            return False
        if ahead.mnemonic.startswith("j"):
            continue
        if ahead.op_str.startswith(register + ","):
            source = ahead.op_str.split(",", 1)[1]
            if ahead.mnemonic == "xor" and source.strip() == register:
                return True
            return (ahead.mnemonic in ("mov", "lea", "xor", "movzx", "movsx")
                    and register not in source)
        if register in ahead.op_str:
            return False
    return False


def calls(listing):
    """Each call's target and the frame slots it takes, by role: the slot in ECX, then in EDX (a
    __fastcall's second argument), then each pushed slot from the last push to the first, so the
    order of the `lea`s doesn't count. A `mov` between registers carries a slot; any other write to
    a register drops it, and a call drops those in registers it may clobber. A slot whose address
    no register or push carries to the call is left out."""
    found = []
    held = {}
    pushed = []
    carried = set()
    for at, text, offsets, depth in listing:
        mnemonic, _, operands = text.partition(" ")
        target = operands.split(",")[0].strip()
        source = operands.split(",")[1].strip() if "," in operands else ""
        if mnemonic == "lea" and offsets:
            held[target] = offsets[0]
            carried.discard(target)
        elif mnemonic == "push" and operands in held:
            pushed.append(held[operands])
            # A register pushed as an argument isn't also the call's register argument.
            carried.add(operands)
        elif mnemonic == "call":
            registers = tuple(held[name] for name in ("ecx", "edx")
                              if name in held and name not in carried)
            found.append((operands, registers + tuple(reversed(pushed))))
            held = {name: slot for name, slot in held.items() if name in SAVED}
            pushed = []
            carried = set()
        elif mnemonic == "mov" and source in held and "[" not in target:
            held[target] = held[source]
            carried.discard(target)
        elif mnemonic not in ("push", "cmp", "test") and target in held:
            # The register now holds something else.
            del held[target]
        # A `rep` prefix names its string instruction; a two-operand imul writes only its target.
        key = mnemonic + " " + operands.split()[0] if mnemonic in ("rep", "repne") else mnemonic
        if "," not in operands or mnemonic in ("rep", "repne") or key in STRING_OPS:
            for written in IMPLICIT.get(key, ()):
                held.pop(written, None)
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
        def __init__(self, mnemonic, op_str, address=0):
            self.mnemonic = mnemonic
            self.op_str = op_str
            self.address = address

    # Code after a jmp takes the depth of the je that reaches it, not of the push above it.
    arms = [Instruction("push", "esi", 0), Instruction("sub", "esp, 8", 1),
            Instruction("je", "0x5", 2), Instruction("push", "1", 3), Instruction("jmp", "0x6", 4),
            Instruction("mov", "eax, dword ptr [esp + 0x10]", 5)]
    arrived = census(arms, -0xc)[1][5][2]
    assert arrived == [4], arrived
    # A mid-body save moves the frame a call returns to, and its pop moves it back; a reload of
    # the pushed register through itself is an argument.
    saved = [Instruction("sub", "esp, 8", 0), Instruction("push", "ebx", 1),
             Instruction("mov", "ebx, eax", 2), Instruction("call", "0x401000", 3),
             Instruction("mov", "eax, dword ptr [esp + 0x10]", 4), Instruction("pop", "ebx", 5),
             Instruction("call", "0x401000", 6),
             Instruction("mov", "eax, dword ptr [esp + 0x10]", 7)]
    depths = [entry[2] for entry in census(saved, -8)[1]]
    assert depths[4] == [4] and depths[7] == [8], depths
    reload = [Instruction("push", "esi"), Instruction("mov", "esi, dword ptr [esi + 4]")]
    assert not saves_register(reload, 0, "esi")
    zeroed = [Instruction("push", "esi"), Instruction("xor", "esi, esi")]
    assert saves_register(zeroed, 0, "esi")
    # A push below a pending argument is an argument too, whatever follows it.
    pending = [Instruction("sub", "esp, 8", 0), Instruction("push", "eax", 1),
               Instruction("push", "esi", 2), Instruction("mov", "esi, ecx", 3),
               Instruction("call", "0x401000", 4),
               Instruction("mov", "eax, dword ptr [esp + 0x10]", 5)]
    after = census(pending, -8)[1][5][2]
    assert after == [8], after

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
    # One call: the client passes -0x20 pushed and -0x14 in ECX, and a lea off a register adds
    # nothing.
    theirs = calls([(0, "lea eax, [esp + 0x10]", [-0x20], -0x30), (1, "push eax", [], -0x30),
                    (2, "lea edx, [eax + 4]", [], -0x34),
                    (3, "lea ecx, [esp + 0x18]", [-0x14], -0x34), (4, "call 0x401000", [], -0x34)])
    assert theirs == [("0x401000", (-0x14, -0x20))], theirs
    # The same roles with the leas the other way round read the same.
    reordered = calls([(0, "lea ecx, [esp + 0x18]", [-0x14], -0x30),
                       (1, "lea eax, [esp + 0x10]", [-0x20], -0x30), (2, "push eax", [], -0x30),
                       (3, "call 0x401000", [], -0x34)])
    assert reordered == theirs, reordered
    # Swapped roles differ.
    swapped = calls([(0, "lea ecx, [esp + 0x10]", [-0x20], -0x30),
                     (1, "lea eax, [esp + 0x18]", [-0x14], -0x30), (2, "push eax", [], -0x30),
                     (3, "call 0x401000", [], -0x34)])
    assert swapped != theirs, swapped
    # EDX is the second register argument; a slot moved into ECX keeps its role; an add drops it;
    # ESI keeps its slot across a call.
    fast = calls([(0, "lea eax, [esp + 0x10]", [-0x20], -0x30), (1, "mov ecx, eax", [], -0x30),
                  (2, "lea edx, [esp + 0x18]", [-0x14], -0x30),
                  (3, "lea esi, [esp + 0x1c]", [-0x10], -0x30),
                  (4, "lea ebx, [esp + 0x20]", [-0xc], -0x30), (5, "add ebx, 4", [], -0x30),
                  (6, "push ebx", [], -0x30), (7, "call 0x401000", [], -0x34),
                  (8, "push esi", [], -0x30), (9, "call 0x401100", [], -0x34)])
    assert fast == [("0x401000", (-0x20, -0x14)), ("0x401100", (-0x10,))], fast
    # A store keeps nothing; cdq and rep stosd clobber their registers.
    clobbered = calls([(0, "lea eax, [esp + 0x10]", [-0x20], -0x30),
                       (1, "mov dword ptr [esp + 4], eax", [-0x2c], -0x30),
                       (2, "lea edx, [esp + 0x14]", [-0x1c], -0x30), (3, "cdq", [], -0x30),
                       (4, "lea edi, [esp + 0x18]", [-0x18], -0x30), (5, "rep stosd", [], -0x30),
                       (6, "push dword ptr [esp + 4]", [-0x2c], -0x30), (7, "push edi", [], -0x34),
                       (8, "call 0x401000", [], -0x38)])
    assert clobbered == [("0x401000", ())], clobbered
    # ECX lea'd and then pushed is one pushed argument, not two.
    once = calls([(0, "lea ecx, [esp + 0x10]", [-0x20], -0x30), (1, "push ecx", [], -0x30),
                  (2, "call 0x401000", [], -0x34)])
    assert once == [("0x401000", (-0x20,))], once
    # ECX pushed, then loaded again, is both a push and the call's register argument.
    twice = calls([(0, "lea ecx, [esp + 0x10]", [-0x20], -0x30), (1, "push ecx", [], -0x30),
                   (2, "lea ecx, [esp + 0x08]", [-0x28], -0x34), (3, "call 0x401000", [], -0x34)])
    assert twice == [("0x401000", (-0x28, -0x20))], twice
    print("selftest passed")
    return 0


if __name__ == "__main__":
    if sys.argv[1:] == ["--selftest"]:
        sys.exit(selftest())

    sys.exit(main())
