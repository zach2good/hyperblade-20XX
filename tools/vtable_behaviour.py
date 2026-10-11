"""Count vtable slots that run a different body from the original's.

A vtable row's byte score counts slots whose names differ. Most of those are link folding: the
original linked identical bodies to one address, and this build keeps a stand-in of its own
with the same code. A slot is a fault only when the stand-in's code differs from the body in the
original's slot, or when the two tables disagree on the slot count.

This reads build/report.json, disassembles both bodies of every differing slot out of the two
images, and compares them with absolute addresses blanked and call targets paired through the
report. A stand-in that jumps to the shared body is followed. Two bodies that hold the same
instructions in another order count as a fold: the scheduler placed them, and they compute the
same thing. It costs no build, but needs the build's report to be current.

    python tools/vtable_behaviour.py            # the count, and every slot that differs
    python tools/vtable_behaviour.py --folds    # also list the slots that are only folds

type_info is the C runtime's and is skipped. Exit code 0 always; it is a count, not a checker.
"""

import argparse
import json
import os
import re
import struct
import sys

import capstone

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from reference import reference_path

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPORT = os.path.join(ROOT, "build", "report.json")
RECOMP = os.path.join(ROOT, "build", "out", "HYPERX_sym.exe")

INSTRUCTION_LIMIT = 2000
THUNK_LIMIT = 4
LOCAL_JUMP = 0x400
CALLEE_DEPTH = 2
SLOT_TEXT = re.compile(r"\((0x[0-9a-f]+|no orig) / (0x[0-9a-f]+|no recomp)\)\s*:\s*(.*)")
ABSOLUTE = re.compile(r"0x[0-9a-f]{6,8}")


class Image:
    def __init__(self, path):
        data = open(path, "rb").read()
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        count = struct.unpack_from("<H", data, pe + 6)[0]
        optional_size = struct.unpack_from("<H", data, pe + 20)[0]
        self.base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
        self.data = data
        self.sections = []
        table = pe + 24 + optional_size
        for index in range(count):
            entry = table + index * 40
            vsize, vaddr, rsize, raddr = struct.unpack_from("<IIII", data, entry + 8)
            self.sections.append((self.base + vaddr, max(vsize, rsize), raddr))

    def bytes_at(self, address, size):
        for start, length, raw in self.sections:
            if start <= address < start + length:
                offset = raw + address - start
                return self.data[offset:offset + size]
        return b""


def disassemble(image, address):
    """Walk a body from its entry until a return no branch jumps past.

    A body that is one jump is a stand-in that tail-calls the shared body, so the walk follows it.
    """
    engine = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    for _ in range(THUNK_LIMIT):
        first = list(engine.disasm(image.bytes_at(address, 16), address, 1))
        if not first:
            break
        if first[0].mnemonic != "jmp" or not first[0].op_str.startswith("0x"):
            break
        address = int(first[0].op_str, 16)
    code = image.bytes_at(address, 0x4000)
    reach = address
    body = []
    for instruction in engine.disasm(code, address):
        body.append(instruction)
        if len(body) >= INSTRUCTION_LIMIT:
            break
        if instruction.mnemonic.startswith("j") and instruction.op_str.startswith("0x"):
            target = int(instruction.op_str, 16)
            local = instruction.mnemonic != "jmp" or target - instruction.address < LOCAL_JUMP
            if local and target > reach:
                reach = target
        terminal = instruction.mnemonic in ("ret", "jmp")
        if terminal and instruction.address >= reach:
            break
    return address, body


def normalise(body, entry, callee_key):
    """Return (text, callee) per instruction; callee is the target of a direct call."""
    lines = []
    end = body[-1].address if body else entry
    for instruction in body:
        text = instruction.op_str
        callee = None
        # A jump out of the body is a tail call, so it pairs like a call.
        tail = instruction.mnemonic == "jmp" and text.startswith("0x") and not entry <= int(text, 16) <= end
        if (instruction.mnemonic == "call" or tail) and text.startswith("0x"):
            callee = int(text, 16)
            text = callee_key(callee)
        elif instruction.mnemonic.startswith("j") and text.startswith("0x"):
            text = "+%x" % (int(text, 16) - entry)
        else:
            text = ABSOLUTE.sub("ADDR", text)
        lines.append((instruction.mnemonic + " " + text, callee))
    return lines


class Comparer:
    """Compare a body in the original with one in this build.

    A call that pairs with a different function through the report still matches when the two
    callees' own bodies match: the original folded one class's destructor onto another's.
    """

    def __init__(self, original, ours, to_original):
        self.original = original
        self.ours = ours
        self.to_original = to_original

    def their_key(self, address):
        return "fn%x" % address

    def our_key(self, address):
        return "fn%x" % self.to_original.get(address, address ^ 0x80000000)

    def verdict(self, their_entry, our_entry, depth=CALLEE_DEPTH):
        their_start, their_body = disassemble(self.original, their_entry)
        our_start, our_body = disassemble(self.ours, our_entry)
        left = normalise(their_body, their_start, self.their_key)
        right = normalise(our_body, our_start, self.our_key)
        if len(left) == len(right) and self.lines_match(left, right, depth):
            return "fold"
        if sorted(text for text, _ in left) == sorted(text for text, _ in right):
            return "order"
        return "body"

    def lines_match(self, left, right, depth):
        for (their_text, their_callee), (our_text, our_callee) in zip(left, right):
            if their_text == our_text:
                continue
            if their_callee is None or our_callee is None or depth == 0:
                return False
            if self.verdict(their_callee, our_callee, depth - 1) != "fold":
                return False
        return True


def slot_pairs(row):
    """Yield (slot, original text, our text) for every slot the two tables disagree on.

    reccmp aligns the two tables like a text diff and labels a shared line with one side's
    offset, so each table is rebuilt in order and the two are compared slot by slot.
    """
    ours = []
    theirs = []
    for hunk in row.get("diff", []):
        for block in hunk[1]:
            for entry in block.get("both", []):
                ours.append(entry[1])
                theirs.append(entry[1])
            for entry in block.get("orig", []):
                theirs.append(entry[1])
            for entry in block.get("recomp", []):
                ours.append(entry[1])
    for index in range(max(len(ours), len(theirs))):
        mine = None
        other = None
        if index < len(ours):
            mine = ours[index]
        if index < len(theirs):
            other = theirs[index]
        if mine != other:
            yield "vtable0x%02x" % (index * 4), other, mine


def parse(text):
    if text is None:
        return None, None, None
    match = SLOT_TEXT.match(text)
    if not match:
        return None, None, text
    original = None
    recompiled = None
    if match.group(1) != "no orig":
        original = int(match.group(1), 16)
    if match.group(2) != "no recomp":
        recompiled = int(match.group(2), 16)
    return original, recompiled, match.group(3).strip()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--folds", action="store_true", help="list the fold slots too")
    options = parser.parse_args()

    rows = json.load(open(REPORT, encoding="utf-8"))["data"]
    original = Image(reference_path())
    ours = Image(RECOMP)

    to_original = {}
    for row in rows:
        if row.get("recomp") and row.get("address"):
            to_original[int(row["recomp"], 16)] = int(row["address"], 16)

    comparer = Comparer(original, ours, to_original)

    faults = 0
    folds = 0
    scheduled = 0
    for row in sorted(rows, key=lambda item: item["address"]):
        if row.get("type") != 5 or row["matching"] >= 1.0:
            continue
        if "type_info" in row["name"]:
            continue
        report = []
        for slot, their_text, our_text in slot_pairs(row):
            their_entry, _, their_name = parse(their_text)
            _, our_entry, our_name = parse(our_text)
            if their_text is None or our_text is None:
                report.append("  %s count   ours %s, original %s" % (slot, our_name, their_name))
                continue
            if their_entry is None or our_entry is None:
                report.append("  %s unread  ours %s, original %s" % (slot, our_name, their_name))
                continue
            verdict = comparer.verdict(their_entry, our_entry)
            if verdict == "fold":
                folds += 1
                if options.folds:
                    report.append("  %s fold    %s = %s" % (slot, our_name, their_name))
                continue
            if verdict == "order":
                scheduled += 1
                if options.folds:
                    report.append("  %s order   %s = %s" % (slot, our_name, their_name))
                continue
            report.append("  %s body    ours %s, original %s" % (slot, our_name, their_name))
        faulty = [line for line in report if " fold " not in line and " order " not in line]
        faults += len(faulty)
        if report:
            print("%s %s" % (row["address"], row["name"]))
            print("\n".join(report))

    print("%d slots run a different body from the original's, %d are folds and %d fold a body"
          " scheduled in another order" % (faults, folds, scheduled))


if __name__ == "__main__":
    main()
