"""Heuristic: find rows where this build rounds an x87 value through memory and the client does not, or the reverse.

The client often squares, scales or compares a value straight from `st(n)`. A rewrite that
stores it to a `float` slot and loads it back rounds it to 24 bits first, so the result can
differ while the score rises. The reverse, a client `fstp`/`fld` pair this build keeps on the
stack, is the same fault the other way.

Every x87 operand is tagged by where it came from: `f` a 4-byte load or exact constant, `d` an
8-byte load, `x` a 10-byte load, `i` an integer, `l` a live result on the stack, `?` unknown.
`d` counts as `l`: a double slot holds a live result exactly at 24 and at 53 bits.
Each arithmetic, compare, square root and integer store becomes an event carrying its operands'
tags. Per operation, the two sides' events are matched as multisets of tag sets, ignoring `?`.
An event left unmatched, paired with an unmatched event of the same operation at about the
same relative position on the other side, is a value one side rounds and the other does not.

It is a heuristic. It reads each function straight through with no control flow, so a value
left on the stack across a branch carries the fall-through's tag, and a call empties the stack
(a float return reads as `?`). Where the sides compute different expressions, two unrelated
leftovers can still pair. Read the listing before spending a build.

A difference only changes a result when the FPU runs at 53 bits, which the CRT sets at startup
and the software renderer path keeps. Direct3D drops it to 24 bits, where a live result is
already a float. A difference on constants that are exact in either width changes nothing.

    python tools/x87_precision.py                  # rows ranked by values that differ
    python tools/x87_precision.py --pairs          # with each differing pair
    python tools/x87_precision.py --row 0x0040c270 --pairs
    python tools/x87_precision.py --selftest       # the recorded pre-fix glide fixture

Exits 2 when the report, this build, its map or the original is missing. --selftest reads
tools/x87_precision_fixture.json, the events of UpdateTerrainPhysics before the glide precision
fix, which squared y and z from frame copies and compared a live flTarget; it exits 1 when those sites stop
reading as differences or when the client's own events compared with themselves do not.
"""

import argparse
import bisect
import csv
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPORT = os.path.join(ROOT, "build", "report.json")
OURS = os.path.join(ROOT, "build", "out", "HYPERX_sym.exe")
OURS_MAP = os.path.join(ROOT, "build", "out", "HYPERX_sym.map")
MAP_SYMBOL = re.compile(r"^\s*0001:[0-9a-f]+\s+\S+\s+([0-9a-f]{8})\s+f\s", re.MULTILINE)
FUNCTIONS = os.path.join(ROOT, "symbols", "functions.csv")
FIXTURE = os.path.join(ROOT, "tools", "x87_precision_fixture.json")

STACK_REG = re.compile(r"^st\((\d)\)$")
ARITH = {"fadd", "fsub", "fsubr", "fmul", "fdiv", "fdivr"}
COMPARE = {"fcom", "fcomp", "fcompp", "fucom", "fucomp", "fucompp"}
CONSTANT = {"fld1", "fldz", "fldpi", "fldl2e", "fldl2t", "fldlg2", "fldln2"}
# A double slot holds a live result exactly at 24 and at 53 bits, the two modes the game runs in.
SAME = {"d": "l"}
# How far apart, as a fraction of each side's event stream, two leftovers may sit and still pair.
NEAR = 0.1
UNARY = {"fsqrt", "fsin", "fcos", "frndint", "f2xm1"}
WIDTH = {"dword": "f", "qword": "d", "xword": "x", "tbyte": "x", "word": "i"}


def operand_tag(text, integer):
    if integer:
        return "i"
    for width, tag in WIDTH.items():
        if text.startswith(width):
            return tag
    return "?"


class Stack:
    def __init__(self):
        self.slots = []

    def get(self, index):
        if index < len(self.slots):
            return self.slots[index]
        return "?"

    def set(self, index, tag):
        while len(self.slots) <= index:
            self.slots.append("?")
        self.slots[index] = tag

    def push(self, tag):
        self.slots.insert(0, tag)
        del self.slots[8:]

    def pop(self):
        if self.slots:
            self.slots.pop(0)


def registers(op_str):
    """The st(n) indices of a register-only operand list, or None when one is memory."""
    if not op_str:
        return []
    found = []
    for part in op_str.split(","):
        match = STACK_REG.match(part.strip())
        if not match:
            return None
        found.append(int(match.group(1)))
    return found


def events(code):
    """(address, op, tags) for every x87 instruction that consumes a value, from capstone code."""
    stack = Stack()
    out = []
    for instruction in code:
        mnemonic = instruction.mnemonic
        ops = instruction.op_str
        if mnemonic == "call":
            stack = Stack()
            continue
        if not mnemonic.startswith("f"):
            continue
        regs = registers(ops)
        if mnemonic in ("fld", "fild"):
            if regs:
                stack.push(stack.get(regs[0]))
            else:
                stack.push(operand_tag(ops, mnemonic == "fild"))
        elif mnemonic in CONSTANT:
            stack.push("f")
        elif mnemonic in ("fst", "fstp"):
            if regs:
                stack.set(regs[0], stack.get(0))
            if mnemonic == "fstp":
                stack.pop()
        elif mnemonic in ("fist", "fistp"):
            out.append((instruction.address, "fist", (stack.get(0),)))
            if mnemonic == "fistp":
                stack.pop()
        elif mnemonic == "fxch":
            other = regs[0] if regs else 1
            top = stack.get(0)
            stack.set(0, stack.get(other))
            stack.set(other, top)
        elif mnemonic in UNARY:
            out.append((instruction.address, mnemonic, (stack.get(0),)))
            stack.set(0, "l")
        elif mnemonic in ("fchs", "fabs"):
            pass
        elif mnemonic.rstrip("p") in ARITH or mnemonic[:2] == "fi" and "f" + mnemonic[2:] in ARITH:
            base = mnemonic.rstrip("p").lstrip("fi")
            base = "f" + base
            if mnemonic.startswith("fi"):
                tags = (stack.get(0), "i")
                destination = 0
            elif regs is None:
                tags = (stack.get(0), operand_tag(ops, False))
                destination = 0
            elif mnemonic.endswith("p"):
                destination = regs[0] if regs else 1
                tags = (stack.get(destination), stack.get(0))
            elif len(regs) == 2:
                destination = regs[0]
                tags = (stack.get(regs[0]), stack.get(regs[1]))
            else:
                destination = 0
                tags = (stack.get(0), stack.get(regs[0] if regs else 1))
            out.append((instruction.address, base.replace("subr", "sub").replace("divr", "div"),
                        tags))
            stack.set(destination, "l")
            if mnemonic.endswith("p"):
                stack.pop()
        elif mnemonic in COMPARE or mnemonic in ("ficom", "ficomp", "ftst"):
            if mnemonic == "ftst":
                tags = (stack.get(0), "f")
            elif mnemonic.startswith("fi"):
                tags = (stack.get(0), "i")
            elif regs is None:
                tags = (stack.get(0), operand_tag(ops, False))
            else:
                tags = (stack.get(0), stack.get(regs[0] if regs else 1))
            out.append((instruction.address, "fcom", tags))
            pops = mnemonic.count("p")
            for _ in range(pops):
                stack.pop()
    return out


def precision(event):
    return sorted(SAME.get(tag, tag) for tag in event[2])


def nearest_pairs(left, right, where, same, limit):
    """Greedy matching of left to right events, closest relative positions first."""
    candidates = sorted(
        (abs(where[a] - where[b]), a, b) for a in left for b in right
        if a[1] == b[1] and same(a, b) and abs(where[a] - where[b]) <= limit)
    used = set()
    pairs = []
    for _, a, b in candidates:
        if a not in used and b not in used:
            used.update((a, b))
            pairs.append((a, b))
    return pairs


def differing(orig, ours):
    """(client event, our event) pairs whose operand tags round differently.

    Events with the same operation and tag set are first matched to each other, nearest
    relative position first, so a reordered sum or a sunk product is no difference. A leftover
    then pairs with the other side's nearest leftover of the same operation within NEAR.
    """
    left = [e for e in orig if "?" not in e[2]]
    right = [e for e in ours if "?" not in e[2]]
    where = {e: i / float(len(orig)) for i, e in enumerate(orig)}
    where.update({e: i / float(len(ours)) for i, e in enumerate(ours)})
    for a, b in nearest_pairs(left, right, where,
                              lambda a, b: precision(a) == precision(b), 1.0):
        left.remove(a)
        right.remove(b)
    return sorted(nearest_pairs(left, right, where, lambda a, b: True, NEAR))


def disassemble(image, data, address, size):
    import capstone

    engine = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    start = address - image.OPTIONAL_HEADER.ImageBase
    return list(engine.disasm(data[start:start + size], address))


def fail(message):
    print("x87_precision: %s" % message, file=sys.stderr)
    return 2


def show(name, address, score, pairs, verbose):
    print("%3d  %6.2f  0x%08x  %s" % (len(pairs), score * 100.0, address, name))
    if not verbose:
        return
    for a, b in pairs:
        print("       client 0x%08x %-5s %-6s ours 0x%08x %-5s %s"
              % (a[0], a[1], ",".join(a[2]), b[0], b[1], ",".join(b[2])))


def selftest():
    if not os.path.exists(FIXTURE):
        return fail("no fixture at %s" % FIXTURE)
    fixture = json.load(open(FIXTURE, encoding="utf-8"))
    ok = True
    for case in fixture["cases"]:
        orig = [(int(e[0], 16), e[1], tuple(e[2])) for e in case["orig"]]
        ours = [(int(e[0], 16), e[1], tuple(e[2])) for e in case["ours"]]
        pairs = differing(orig, ours)
        control = differing(orig, list(orig))
        found = sorted({a[0] for a, _ in pairs})
        expected = [int(x, 16) for x in case["expect"]]
        missing = [x for x in expected if x not in found]
        print("%s: %d pair(s), expected client sites %s"
              % (case["name"], len(pairs), "found" if not missing else
                 "MISSING %s" % ", ".join("0x%08x" % x for x in missing)))
        if control:
            print("%s: the client's events differ from themselves" % case["name"])
        ok = ok and not missing and not control and len(pairs) >= case["min_pairs"]
    return 0 if ok else 1


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pairs", action="store_true", help="print each differing pair")
    parser.add_argument("--row", default="", help="only this client address")
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()

    if args.selftest:
        return selftest()

    if not os.path.exists(REPORT):
        return fail("no %s; run reccmp's asmcmp with --json first" % REPORT)
    if not os.path.exists(OURS) or not os.path.exists(OURS_MAP):
        return fail("no %s or its map; build first" % OURS)

    import pefile
    from reference import reference_path

    reference = reference_path(required=False)
    if not reference or not os.path.exists(reference):
        return fail("no original executable; see tools/reference.py")

    sizes = {}
    with open(FUNCTIONS, encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            sizes[int(row["address"], 16)] = int(row["size"])

    rows = json.load(open(REPORT, encoding="utf-8"))["data"]
    # Every function start in this build bounds the one before it; the report alone misses
    # functions with no row.
    starts = {int(r["recomp"], 16) for r in rows if r.get("recomp")}
    starts.update(int(m, 16) for m in MAP_SYMBOL.findall(open(OURS_MAP).read()))
    starts = sorted(starts)
    only = int(args.row, 16) if args.row else None

    images = []
    for path in (reference, OURS):
        image = pefile.PE(path, fast_load=True)
        images.append((image, image.get_memory_mapped_image()))

    found = []
    for row in rows:
        address = int(row["address"], 16)
        if only is not None and address != only:
            continue
        if row.get("matching", 0) >= 1.0:
            continue
        if not row.get("recomp") or address not in sizes:
            continue
        recomp = int(row["recomp"], 16)
        following = bisect.bisect_right(starts, recomp)
        extent = starts[following] - recomp if following < len(starts) else sizes[address] * 2
        orig = events(disassemble(images[0][0], images[0][1], address, sizes[address]))
        ours = events(disassemble(images[1][0], images[1][1], recomp, extent))
        pairs = differing(orig, ours)
        if pairs:
            found.append((len(pairs), row["name"], address, row["matching"], pairs))

    found.sort(key=lambda item: (-item[0], item[2]))
    for count, name, address, score, pairs in found:
        show(name, address, score, pairs, args.pairs)
    print("%d row(s) with a rounding difference" % len(found))
    return 0


if __name__ == "__main__":
    sys.exit(main())
