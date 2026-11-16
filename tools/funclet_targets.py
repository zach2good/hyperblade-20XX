"""Name each unwind state's destructor on both sides, and print the states that differ.

A function with a C++ EH frame pushes `push -1; push <handler>` near its start, and the handler
loads `mov eax, <FuncInfo>` in `.text$x`, whose unwind map lists one action per state. An action's
first `call` or `jmp` is the destructor it runs. A state matches when build/report.json pairs the
original's destructor with ours, or with a copy HYPERX.exe folds ours with. The rest print grouped
by the pair of names, with a count, and so do parents whose state counts differ. A state whose
destructors pair but whose object operand differs, another frame slot or member, prints on its
own. Read only.

    python tools/funclet_targets.py
    python tools/funclet_targets.py --list      # also each differing state's parent
    python tools/funclet_targets.py --selftest
"""

import bisect
import collections
import json
import re
import struct
import sys
from pathlib import Path

import capstone
import yaml
from reccmp.formats import detect_image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import foldcmp  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "build" / "out"
TEXT_START = 0x00401000
FUNCINFO_MAGIC = 0x19930520
MOV_EAX = 0xB8
JMP = 0xE9
CALL = 0xE8

# Enough for an action's few loads before its first branch.
BRANCH_WINDOW = 32
DISASSEMBLER = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

ADDRESS = re.compile(r"0x[0-9a-f]{6,8}")

SECTION_LINE = re.compile(r"^ 0001:([0-9a-f]{8}) ([0-9a-f]{8})H \.text\$x\s")


def first_branch(image, addr):
    """Answers the target of the action's first `call` or `jmp`, or None, and the instructions
    before it, which name the object: a `lea ecx, [ebp - N]` slot or an `add ecx, N` member. A
    conditional jump inside the action keeps its mnemonic only, since its target never pairs."""
    code = image.read(addr, BRANCH_WINDOW)
    before = []
    for inst in DISASSEMBLER.disasm(code, addr):
        if inst.mnemonic in ("call", "jmp"):
            if inst.op_str.startswith("0x"):
                return int(inst.op_str, 16), tuple(before)

            return None, tuple(before)

        if inst.mnemonic.startswith("j"):
            before.append(inst.mnemonic)
        else:
            before.append("%s %s" % (inst.mnemonic, inst.op_str))

    return None, tuple(before)


def mapped(operands, pairs, twins):
    """Answers instructions with each address read as our copy, then as the lowest copy in its
    fold group, so a folded twin reads as the copy the original's address pairs with. Pass
    {} as `pairs` for our own instructions."""
    def ours(found):
        addr = pairs.get(int(found.group(0), 16), int(found.group(0), 16))
        return "%#x" % min(twins.get(addr, {addr}))

    return tuple(ADDRESS.sub(ours, line) for line in operands)


def unwind_targets(image, funclets_start, funclets_end, starts):
    """Answers {parent function: [each state's (destructor, object operands)]} for each handler
    in the funclet range whose FuncInfo carries the magic and whose parent pushes it. The parent
    is the function in the sorted `starts` that holds the push."""
    funclets = image.read(funclets_start, funclets_end - funclets_start)
    text = image.read(TEXT_START, funclets_start - TEXT_START)
    found = {}
    # A handler is `mov eax, imm32` and a 5-byte `jmp`: ten bytes.
    for at in range(len(funclets) - 9):
        if funclets[at] != MOV_EAX or funclets[at + 5] != JMP:
            continue

        info = struct.unpack("<I", funclets[at + 1:at + 5])[0]
        try:
            magic, states, unwind_map = struct.unpack("<III", image.read(info, 12))
        except Exception:  # noqa: BLE001 - an immediate that isn't a mapped address
            continue

        if magic != FUNCINFO_MAGIC:
            continue

        push = text.find(b"\x6a\xff\x68" + struct.pack("<I", funclets_start + at))
        if push < 0:
            continue

        targets = []
        for state in range(states):
            action = struct.unpack("<iI", image.read(unwind_map + 8 * state, 8))[1]
            targets.append(first_branch(image, action) if action else (None, ()))

        found[starts[bisect.bisect_right(starts, TEXT_START + push) - 1]] = targets

    return found


def funclet_range(map_path):
    for line in map_path.read_text(errors="replace").splitlines():
        found = SECTION_LINE.match(line)
        if found:
            start = TEXT_START + int(found.group(1), 16)
            return start, start + int(found.group(2), 16)

    raise SystemExit("%s has no .text$x" % map_path)


def differing(theirs, ours, pairs, twins):
    """Answers (original parent, state, original target, our target) for each state whose
    destructors don't pair, through `twins`, which maps our address to its fold group; the same
    with the two sides' object operands for a state whose destructors pair but whose objects
    differ; and (original parent, None, their count, our count) for a parent whose state counts
    differ."""
    found = []
    for parent, targets in theirs.items():
        mine = ours.get(pairs.get(parent))
        if mine is None:
            continue

        if len(targets) != len(mine):
            found.append((parent, None, len(targets), len(mine)))
            continue

        for state, ((orig, theirs_object), (recomp, our_object)) in enumerate(zip(targets, mine)):
            if orig is None and recomp is None:
                continue

            group = twins.get(recomp, {recomp})
            if not any(pairs.get(orig) == copy for copy in group):
                found.append((parent, state, orig, recomp))
            elif mapped(theirs_object, pairs, twins) != mapped(our_object, {}, twins):
                found.append((parent, state, theirs_object, our_object))

    return found


def main():
    rows = json.loads((ROOT / "build" / "report.json").read_text())["data"]
    pairs = {int(row["address"], 16): int(row["recomp"], 16) for row in rows if row.get("recomp")}
    names = {int(row["address"], 16): row["name"] for row in rows}
    symbols = {}
    for line in (OUT / "HYPERX_sym.map").read_text(errors="replace").splitlines():
        found = foldcmp.OBJECT_LINE.match(line)
        if found:
            symbols.setdefault(int(found.group(2), 16), found.group(1))

    measured = (OUT / "HYPERX.map").read_text(errors="replace").splitlines()
    groups = foldcmp.fold_groups(foldcmp.read_lines(measured),
                                 foldcmp.read_map(OUT / "HYPERX_sym.map"),
                                 foldcmp.relabelled(measured))
    twins = {addr: group for group in groups for addr in group}

    user = yaml.safe_load((ROOT / "reccmp-user.yml").read_text())
    original = detect_image(user["targets"]["HYPERX"]["path"])
    # functions.csv lists every function start, where the report lists only the paired ones.
    with (ROOT / "symbols" / "functions.csv").open(encoding="utf-8") as handle:
        starts = sorted(int(line.split(",")[0], 16) for line in handle if line.startswith("0x"))

    theirs = unwind_targets(original, *original_funclets(original), starts)
    ours = unwind_targets(detect_image(str(OUT / "HYPERX_sym.exe")),
                          *funclet_range(OUT / "HYPERX_sym.map"), sorted(symbols))

    found = differing(theirs, ours, pairs, twins)
    counts = collections.Counter()
    parents = collections.defaultdict(list)
    for parent, state, orig, recomp in found:
        if state is None:
            print("state counts differ: %s has %d, ours %d"
                  % (names.get(parent, "%#x" % parent), orig, recomp))
            continue

        if isinstance(orig, tuple):
            print("object differs: %s state %d: %s / %s"
                  % (names.get(parent, "%#x" % parent), state, "; ".join(orig), "; ".join(recomp)))
            continue

        key = (names.get(orig, "%#x" % orig if orig else "-"),
               symbols.get(recomp, "%#x" % recomp if recomp else "-"))
        counts[key] += 1
        parents[key].append("%s state %d" % (names.get(parent, "%#x" % parent), state))

    for (orig, recomp), count in counts.most_common():
        print("%4d  %s  /  %s" % (count, orig, recomp))
        if "--list" in sys.argv:
            for parent in parents[(orig, recomp)]:
                print("        %s" % parent)

    print("%d parent(s) in the original, %d here; %d state(s) differ"
          % (len(theirs), len(ours), len(found)))
    return 0


def original_funclets(original):
    """The original's `.text$x` runs from its first `Unwind@` in symbols/functions.csv to the end
    of `.text`."""
    unwinds = [int(line.split(",")[0], 16) for line in (ROOT / "symbols" / "functions.csv").open()
               if ",Unwind@" in line]
    text = next(section for section in original.sections if section.name == ".text")
    return min(unwinds), text.virtual_address + text.virtual_size


def selftest():
    """Checks the walk on a toy image: a parent whose push comes after its frame setup, a handler
    at the range's end, two states; then a pair, a fold twin, a mismatch and a count mismatch."""
    class Image:
        def __init__(self, blocks):
            self.blocks = blocks

        def read(self, addr, size):
            for start, data in self.blocks.items():
                if start <= addr and addr + size <= start + len(data):
                    return data[addr - start:addr - start + size]

            raise ValueError("unmapped")

    def words(*values):
        return b"".join(value.to_bytes(4, "little") for value in values)

    # The parent at 0x401000 opens `push ebp; mov ebp, esp`, then pushes the handler at 0x402010,
    # the last ten bytes of the range, which loads FuncInfo 0x403000. State 0 runs the action at
    # 0x402000, `call 0x401100`; state 1 has no action.
    text = bytes([0x55, 0x8B, 0xEC, 0x6A, 0xFF, 0x68]) + words(0x402010) + bytes(0x1000 - 10)
    funclets = bytes([CALL]) + struct.pack("<i", 0x401100 - 0x402005) + bytes(11)
    funclets += bytes([MOV_EAX]) + words(0x403000) + bytes([JMP]) + bytes(4)
    image = Image({0x401000: text, 0x402000: funclets + bytes(0x20),
                   0x403000: words(FUNCINFO_MAGIC, 2, 0x403100),
                   0x403100: words(0, 0x402000, 0, 0)})
    found = unwind_targets(image, 0x402000, 0x402000 + len(funclets), [0x400000, 0x401000])
    assert found == {0x401000: [(0x401100, ()), (None, ())]}, found

    # Our copy pairs; then our destructor is another that folds with the paired one.
    nothing = (None, ())
    pairs = {0x401000: 0x501000, 0x401100: 0x501100, 0x4D0000: 0x5D0000}
    assert differing(found, {0x501000: [(0x501100, ()), nothing]}, pairs, {}) == []
    twins = {0x501200: {0x501100, 0x501200}}
    assert differing(found, {0x501000: [(0x501200, ()), nothing]}, pairs, twins) == []
    expected = [(0x401000, 0, 0x401100, 0x501300)]
    assert differing(found, {0x501000: [(0x501300, ()), nothing]}, pairs, twins) == expected
    assert differing(found, {0x501000: [(0x501100, ())]}, pairs, {}) == [(0x401000, None, 2, 1)]

    # The same destructor on another member is a difference; a paired address in the operand isn't.
    member = {0x401000: [(0x401100, ("add ecx, 0xe8",)), nothing]}
    moved = {0x501000: [(0x501100, ("add ecx, 0x114",)), nothing]}
    expected = [(0x401000, 0, ("add ecx, 0xe8",), ("add ecx, 0x114",))]
    assert differing(member, moved, pairs, {}) == expected
    named = {0x401000: [(0x401100, ("mov ecx, 0x4d0000",)), nothing]}
    ours = {0x501000: [(0x501100, ("mov ecx, 0x5d0000",)), nothing]}
    assert differing(named, ours, pairs, {}) == []
    folded = {0x501000: [(0x501100, ("mov ecx, 0x5e0000",)), nothing]}
    assert differing(named, folded, pairs, {0x5E0000: {0x5D0000, 0x5E0000}}) == []
    print("selftest passed")
    return 0


if __name__ == "__main__":
    if sys.argv[1:] == ["--selftest"]:
        raise SystemExit(selftest())

    raise SystemExit(main())
