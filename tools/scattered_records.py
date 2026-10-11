"""Find globals the original reads as one record and this tree declares as separate variables.

Face::ClipPolygon takes the address of the face staging count and reads the mask four bytes on
and the vertices twelve on. The tree declared the three as separate globals, each under its own
GLOBAL marker, so each direct operand matched and the byte score saw nothing. The linker put them
apart, and every clip read garbage and wrote over whatever followed the count.

This reads the original's code, not the tree's. For each GLOBAL marker it finds the
instructions that take the global's address (`push X`, `mov reg, X`, `lea reg, [X]`) and follows
that pointer: through register copies and offsets, into a callee through a register or a stack
argument, through `rep stos`/`rep movs` counts, and through a memset or memcpy call's length.
Where the original touches bytes past the global's declared size (the size comes from this
build's PDB), it names what lives there and asks this build's map whether it still sits at the
same distance. A global the PDB gives no size, such as one first declared as an extern array with
no bound, is reported and then measured by the gap to the next symbol in this build's map, so the
reach checks still run on it.

    OVERLAP     the tree declares X larger than the gap to the next marked global Y. Two names
                for one set of bytes, which the linker then places apart.
    BOUND       a loop starts at X and ends on the address of Y, which the original keeps right
                after X, and this build does not.
    READS_PAST  the original reaches from X into Y, a separate variable here. Where this build
                happens to keep Y at the same distance the row says so; nothing holds it there.
    SHORT       the original reaches past X's declared end, and nothing is marked there. This
                includes a direct operand at X, or a scaled index, wider than the declaration.
    UNSIZED     the PDB records X with no size or size 0. The tree never gave it a bound, so
                reccmp compares nothing past its first byte.
    ESCAPES     X's address goes where this cannot follow (an import, a store, a pointer walk)
                and the next marked global Y abuts it and has moved. Printed with --all only;
                each is a question for Ghidra.

    python tools/scattered_records.py                     # measured build's map
    python tools/scattered_records.py --map build-bringup/out/HYPERX.map
    python tools/scattered_records.py --all               # also the ESCAPES review list
    python tools/scattered_records.py --at 0x004cb810     # every use of one global

The pointer walk is linear and bounded, so a READS_PAST row is evidence and an empty list is
not proof. A row that has been read in Ghidra and is not a record goes in
symbols/scattered_records.csv with the reason. The pool counts are two names for one word on
purpose, as tools/check_globals.py allows. Needs the original (reccmp-user.yml), a map and the
HYPERX_sym.pdb beside it. Exits 1 when an unregistered OVERLAP, BOUND, READS_PAST, SHORT or
UNSIZED row stands.
"""
import argparse
import bisect
import collections
import csv
import glob
import os
import re
import sys

import capstone
from capstone import x86

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))

import global_drift  # noqa: E402

MAP = os.path.join(ROOT, "build", "out", "HYPERX.map")
FUNCTIONS = os.path.join(ROOT, "symbols", "functions.csv")
REGISTER = os.path.join(ROOT, "symbols", "scattered_records.csv")

OTHER_MARKER = re.compile(r"//\s*(FUNCTION|LIBRARY|STUB|VTABLE|STRING):\s*HYPERX\s+0x([0-9a-fA-F]+)")

# How far a pointer is followed: instructions per function, and callee depth.
SCAN_LIMIT = 400
CALL_DEPTH = 3

VOLATILE = ("eax", "ecx", "edx")
FULL = {"al": "eax", "ah": "eax", "ax": "eax", "cl": "ecx", "ch": "ecx", "cx": "ecx",
        "dl": "edx", "dh": "edx", "dx": "edx", "bl": "ebx", "bh": "ebx", "bx": "ebx",
        "si": "esi", "di": "edi", "bp": "ebp", "sp": "esp"}


def full(name):
    return FULL.get(name, name)


class Image:
    """The original's code and data, and every function's extent."""

    def __init__(self):
        from check_short_arrays import original

        self.image = original()
        self.low = self.image.get_section_by_name(".rdata").virtual_range.start
        self.high = self.image.get_section_by_name(".data").virtual_range.stop
        self.cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.cs.detail = True
        self.functions = {}
        self.cache = {}

        for row in csv.DictReader(open(FUNCTIONS)):
            size = int(row["size"] or 0)

            if size > 0:
                self.functions[int(row["address"], 16)] = (size, row["name"])

    def code(self, start):
        if start not in self.cache:
            size = self.functions[start][0]
            self.cache[start] = list(self.cs.disasm(self.image.read(start, size), start))

        return self.cache[start]


def reg_name(cs, reg):
    return full(cs.reg_name(reg)) if reg else None


def follow(img, start, index, held, depth, seen):
    """Offsets from the global that code touches, starting at instruction `index` of `start`.

    `held` maps a register (or an argument slot, "arg0".."argN") to the offset from the global
    it holds. Returns (touched, notes): touched is a list of (low, high) byte ranges and notes a
    set of reasons the walk lost the pointer.
    """
    touched = []
    notes = set()
    insns = img.code(start)
    regs = {k: v for k, v in held.items() if not k.startswith("arg")}
    args = {int(k[3:]): v for k, v in held.items() if k.startswith("arg")}
    # The stack depth since entry, and the depth ebp was set at.
    depth_esp = 0
    frame = None
    pushes = []

    for ins in insns[index:index + SCAN_LIMIT]:
        mnem = ins.mnemonic
        ops = ins.operands

        # Stack argument loads: `mov reg, [esp + d]` or `[ebp + d]` naming an argument slot.
        if mnem == "mov" and len(ops) == 2 and ops[0].type == x86.X86_OP_REG \
                and ops[1].type == x86.X86_OP_MEM and args:
            base = reg_name(img.cs, ops[1].mem.base)
            slot = None

            if base == "esp" and not ops[1].mem.index:
                slot = ops[1].mem.disp - depth_esp - 4
            elif base == "ebp" and frame is not None and not ops[1].mem.index:
                slot = ops[1].mem.disp - frame - 4

            if slot is not None and slot >= 0 and slot % 4 == 0 and slot // 4 in args:
                regs[reg_name(img.cs, ops[0].reg)] = args[slot // 4]
                continue

        # Every memory operand through a held register is a touch.
        for op in ops:
            if op.type != x86.X86_OP_MEM:
                continue

            base = reg_name(img.cs, op.mem.base)

            if base in regs and base not in ("esp", "ebp"):
                low = regs[base] + op.mem.disp

                if op.mem.index:
                    notes.add("indexed at 0x%08x" % ins.address)
                elif mnem != "lea":
                    touched.append((low, low + op.size))

        if mnem == "lea" and ops[0].type == x86.X86_OP_REG:
            dest = reg_name(img.cs, ops[0].reg)
            base = reg_name(img.cs, ops[1].mem.base)

            if base in regs and not ops[1].mem.index:
                regs[dest] = regs[base] + ops[1].mem.disp
            else:
                regs.pop(dest, None)
            continue

        if mnem == "push":
            depth_esp += 4
            value = None

            if ops[0].type == x86.X86_OP_REG:
                value = regs.get(reg_name(img.cs, ops[0].reg))

            pushes.append(value)
            continue

        if mnem == "pop":
            depth_esp -= 4

            if pushes:
                pushes.pop()

            if ops[0].type == x86.X86_OP_REG:
                regs.pop(reg_name(img.cs, ops[0].reg), None)
            continue

        if mnem in ("sub", "add") and ops[0].type == x86.X86_OP_REG:
            dest = reg_name(img.cs, ops[0].reg)

            if dest == "esp" and ops[1].type == x86.X86_OP_IMM:
                depth_esp += ops[1].imm if mnem == "sub" else -ops[1].imm
                continue

            if dest in regs:
                if ops[1].type == x86.X86_OP_IMM:
                    regs[dest] += ops[1].imm if mnem == "add" else -ops[1].imm
                    notes.add("stepped at 0x%08x" % ins.address)
                else:
                    notes.add("walked at 0x%08x" % ins.address)
                    regs.pop(dest)
            continue

        if mnem == "mov" and ops[0].type == x86.X86_OP_REG:
            dest = reg_name(img.cs, ops[0].reg)

            if dest == "ebp" and ops[1].type == x86.X86_OP_REG \
                    and reg_name(img.cs, ops[1].reg) == "esp":
                frame = depth_esp
                continue

            if ops[1].type == x86.X86_OP_REG and reg_name(img.cs, ops[1].reg) in regs:
                regs[dest] = regs[reg_name(img.cs, ops[1].reg)]
            else:
                regs.pop(dest, None)
            continue

        if mnem == "mov" and ops[0].type == x86.X86_OP_MEM and ops[1].type == x86.X86_OP_REG \
                and reg_name(img.cs, ops[1].reg) in regs:
            if reg_name(img.cs, ops[0].mem.base) not in ("esp", "ebp"):
                notes.add("stored at 0x%08x" % ins.address)
            continue

        if mnem.startswith("rep"):
            if "edi" in regs and ("stos" in mnem or "movs" in mnem):
                notes.add("rep at 0x%08x" % ins.address)
            if "esi" in regs and "movs" in mnem:
                notes.add("rep at 0x%08x" % ins.address)
            continue

        if mnem == "call":
            passed = {}

            for reg in ("ecx", "edx"):
                if reg in regs:
                    passed[reg] = regs[reg]

            for slot, value in enumerate(reversed(pushes)):
                if value is not None:
                    passed["arg%d" % slot] = value

            if passed:
                target = ops[0].imm if ops[0].type == x86.X86_OP_IMM else None

                if target in img.functions and depth > 0 and (target, tuple(sorted(passed.items()))) not in seen:
                    seen.add((target, tuple(sorted(passed.items()))))
                    more, why = follow(img, target, 0, passed, depth - 1, seen)
                    touched.extend(more)
                    notes.update(why)
                elif target not in img.functions:
                    notes.add("passed to %s at 0x%08x" % (ins.op_str, ins.address))

            for reg in VOLATILE:
                regs.pop(reg, None)

            pushes = []
            continue

        if mnem in ("ret", "retn") or (mnem == "jmp" and ops[0].type != x86.X86_OP_IMM):
            break

        # Any other write to a held register ends what it held.
        if ops and ops[0].type == x86.X86_OP_REG and mnem not in ("cmp", "test", "push"):
            regs.pop(reg_name(img.cs, ops[0].reg), None)

        if not regs and not args:
            break

    return touched, notes


def rep_reach(insns, i):
    """Bytes a `rep stos`/`rep movs` near instruction i covers, from its `mov ecx, imm`."""
    count = None

    for t in insns[max(0, i - 6):i + 10]:
        if t.mnemonic == "mov" and t.op_str.startswith("ecx, 0x"):
            try:
                count = int(t.op_str.split(", ")[1], 16)
            except ValueError:
                count = None

        if t.mnemonic.startswith("rep") and t.address > insns[i].address and count:
            return count * (4 if t.mnemonic.endswith("d") else 1)

    return None


def read_sizes(pdb):
    from reccmp.cvdump.analysis import CvdumpAnalysis, EntityType
    from reccmp.cvdump.runner import Cvdump

    if not os.path.exists(pdb):
        sys.exit("no %s; build first" % pdb)

    parsed = Cvdump(pdb).globals().types().publics().run()

    # A global the PDB types as an array with no bound comes back as size 0 or no size at all.
    return {node.friendly_name: node.confirmed_size or 0 for node in CvdumpAnalysis(parsed).nodes
            if node.node_type == EntityType.DATA}


def read_gaps(path):
    """Each map symbol's distance to the next symbol above it, by the name read_map gives it."""
    rows = []

    for line in open(path, encoding="latin-1"):
        found = global_drift.MAP_ROW.match(line)

        if found:
            rows.append(int(found.group(2), 16))

    starts = sorted(set(rows))
    ours = global_drift.read_map()
    gaps = {}

    for name, address in ours.items():
        at = bisect.bisect_right(starts, address)

        if at < len(starts):
            gaps[name] = starts[at] - address

    return gaps


def other_markers():
    found = set()
    library = set()

    for path in glob.glob(os.path.join(ROOT, "src", "**", "*.*"), recursive=True):
        if not path.endswith((".h", ".cpp", ".c")):
            continue

        for hit in OTHER_MARKER.finditer(open(path, encoding="utf-8", errors="replace").read()):
            address = int(hit.group(2), 16)
            found.add(address)

            if hit.group(1) == "LIBRARY":
                library.add(address)

    return found, library


def read_register():
    """(kind, address) pairs that have been read and are not records."""
    if not os.path.exists(REGISTER):
        return set()

    lines = [line for line in open(REGISTER, encoding="utf-8") if not line.startswith("#")]

    return set((row["kind"], int(row["address"], 16)) for row in csv.DictReader(lines))


def uses_of(img, owner):
    """Every non-direct reference in the original's code to a marked global, by global."""
    out = collections.defaultdict(list)

    for start in sorted(img.functions):
        insns = img.code(start)

        for i, ins in enumerate(insns):
            for op in ins.operands:
                if op.type == x86.X86_OP_IMM and img.low <= op.imm < img.high:
                    value = op.imm
                    kind = "addr"
                elif op.type == x86.X86_OP_MEM and img.low <= op.mem.disp < img.high:
                    value = op.mem.disp

                    if op.mem.index:
                        kind = "scaled%d" % op.mem.scale
                    elif op.mem.base:
                        kind = "indexed"
                    elif ins.mnemonic == "lea":
                        kind = "addr"
                    else:
                        kind = "direct%d" % op.size
                else:
                    continue

                home = owner(value)

                if home is not None:
                    out[home].append((start, i, kind, value - home))

    return out


def reach_of(img, start, i, offset):
    """Byte ranges the original touches from one address-taking instruction, and the notes."""
    insns = img.code(start)
    ins = insns[i]
    ops = ins.operands
    touched = []
    notes = set()

    if ins.mnemonic in ("mov", "lea") and ops[0].type == x86.X86_OP_REG:
        dest = reg_name(img.cs, ops[0].reg)
        more, why = follow(img, start, i + 1, {dest: offset}, CALL_DEPTH, set())
        touched.extend(more)
        notes.update(why)
        count = rep_reach(insns, i)

        if count and dest in ("edi", "esi"):
            touched.append((offset, offset + count))
            notes.discard("rep at 0x%08x" % ins.address)
    elif ins.mnemonic == "push":
        # The pushed address is an argument to the next call; count the pushes after it.
        after = 0

        for t in insns[i + 1:i + 12]:
            if t.mnemonic == "push":
                after += 1
            elif t.mnemonic == "pop":
                after -= 1
            elif t.mnemonic == "call":
                target = t.operands[0].imm if t.operands[0].type == x86.X86_OP_IMM else None

                if target in img.functions and after == 0                         and img.functions[target][1].lstrip("_") in ("memset", "memcpy", "memmove"):
                    # The length is the third argument, pushed two before the address.
                    length = insns[i - 2] if i >= 2 else None

                    if length is not None and length.mnemonic == "push"                             and length.operands[0].type == x86.X86_OP_IMM:
                        touched.append((offset, offset + length.operands[0].imm))
                elif target in img.functions:
                    more, why = follow(img, target, 0, {"arg%d" % after: offset},
                                       CALL_DEPTH - 1, set())
                    touched.extend(more)
                    notes.update(why)
                else:
                    notes.add("passed to %s at 0x%08x" % (t.op_str, t.address))
                break
    elif ins.mnemonic == "cmp":
        notes.add("bound")
    elif ops and ops[0].type == x86.X86_OP_MEM:
        notes.add("stored at 0x%08x" % ins.address)
    else:
        notes.add("%s at 0x%08x" % (ins.mnemonic, ins.address))

    return touched, notes


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--map", default=MAP)
    parser.add_argument("--all", action="store_true", help="also print the ESCAPES review list")
    parser.add_argument("--at", help="print every use of the global marked at this address")
    args = parser.parse_args()

    if not os.path.exists(args.map):
        sys.exit("no %s; build first" % args.map)

    global_drift.MAP = args.map
    ours = global_drift.read_map()
    marked = global_drift.read_markers()
    # Sizes come from the PDB beside the map, so the bring-up map is read with its own types.
    declared = read_sizes(os.path.join(os.path.dirname(os.path.abspath(args.map)), "HYPERX_sym.pdb"))
    gaps = read_gaps(args.map)
    img = Image()
    others, library = other_markers()

    byaddr = {}

    for name, (address, unused) in marked.items():
        byaddr.setdefault(address, name)

    addrs = sorted(byaddr)
    bounds = sorted(set(addrs) | others)

    def owner(value):
        at = bisect.bisect_right(bounds, value) - 1

        if at < 0 or bounds[at] not in byaddr:
            return None

        return bounds[at]

    def home_of(address):
        at = bisect.bisect_right(addrs, address) - 1
        return addrs[at] if at >= 0 else None

    def moved(x, y):
        """Whether this build keeps y at the original's distance from x."""
        nx = byaddr[x]
        ny = byaddr[y]

        if nx not in ours or ny not in ours:
            return False

        return ours[ny] - ours[nx] != y - x

    rows = []
    sizes = {}

    # UNSIZED: a symbol this build defines that the PDB gives no size. The map gap stands in for
    # the size so the reach checks below still run, but it is not a declaration, so it takes no
    # part in OVERLAP.
    for x in addrs:
        name = byaddr[x]

        if declared.get(name):
            sizes[name] = declared[name]
        elif name in ours and gaps.get(name):
            sizes[name] = gaps[name]
            why = "the PDB records size %d; the map gap to the next symbol is %d"
            rows.append(("UNSIZED", x, 0, [], [why % (declared.get(name, 0), gaps[name])]))

    # OVERLAP: straight from the declarations.
    for at, x in enumerate(addrs[:-1]):
        size = declared.get(byaddr[x])

        if not size:
            continue

        inside = [y for y in addrs[at + 1:] if y < x + size]
        pools = [marked[byaddr[a]][1] for a in [x] + inside]

        if inside and not all(os.path.basename(p) == "poolcounts.h" for p in pools):
            rows.append(("OVERLAP", x, size, [(y, "declared inside it") for y in inside[:6]],
                         []))

    uses = uses_of(img, owner)
    escapes = []

    for x in sorted(uses):
        name = byaddr[x]
        size = sizes.get(name)

        # A file-scope static has no map row, but its PDB size still holds it to the original.
        if not size:
            continue

        past = collections.defaultdict(set)
        lost = set()
        # The furthest byte the original reaches that still lies inside X's own span, and who.
        furthest = [0, None]

        def reach(high, who):
            if high > furthest[0] and owner(x + high - 1) == x:
                furthest[0] = high
                furthest[1] = who

        for start, i, kind, offset in uses[x]:
            if start in library:
                continue

            fn = img.functions[start][1]

            # A direct operand further on names its own address, often an unmarked constant, so
            # only one at X itself says how wide X is.
            if kind.startswith("direct"):
                if offset == 0:
                    reach(int(kind[len("direct"):]), "%s reads it at 0x%08x"
                          % (fn, img.code(start)[i].address))
                continue

            if kind.startswith("scaled"):
                scale = int(kind[len("scaled"):])
                reach(offset + scale, "%s indexes it at 0x%08x"
                      % (fn, img.code(start)[i].address))

                if offset >= size:
                    past[offset].add("%s indexes 0x%08x" % (fn, x + offset))
                elif size <= scale:
                    past[size].add("%s indexes it as an array of %d byte elements"
                                   % (fn, scale))
                continue

            if kind == "indexed":
                if offset >= size:
                    past[offset].add("%s indexes 0x%08x" % (fn, x + offset))
                continue

            ins = img.code(start)[i]

            if ins.mnemonic == "cmp" and offset <= size:
                continue

            touched, notes = reach_of(img, start, i, offset)

            if offset >= size and ins.mnemonic != "cmp":
                past[offset].add("%s takes 0x%08x" % (fn, x + offset))

            for low, high in touched:
                reach(high, "%s reaches +0x%x..+0x%x" % (fn, low, high))

                if high > size or low < 0:
                    past[max(low, size) if low >= 0 else low].add(
                        "%s reaches +0x%x..+0x%x" % (fn, low, high))

            for note in notes:
                if note != "bound":
                    lost.add("%s: %s" % (fn, note))

        reached = []

        for offset in sorted(past):
            target = x + offset
            y = home_of(target)

            if y is None:
                continue

            if y == x:
                reached.append(("SHORT", target, sorted(past[offset])))
            elif moved(x, y) or name not in ours or byaddr[y] not in ours:
                reached.append(("READS_PAST", target, sorted(past[offset])))
            else:
                # The layout happens to keep y where the original reads it, which nothing holds.
                reached.append(("READS_PAST", target,
                                sorted(past[offset]) + ["adjacent here by chance"]))

        # The declaration against the original's furthest reach inside X's span. With a real
        # size this repeats what the reach walk found; it earns its keep on an unsized global,
        # whose stand-in size is the map gap.
        if furthest[0] > declared.get(name, 0) and not any(k == "SHORT" for k, t, w in reached):
            reached.append(("SHORT", x + declared.get(name, 0),
                            ["declared %d bytes, the original reaches +0x%x: %s"
                             % (declared.get(name, 0), furthest[0], furthest[1])]))

        for kind in ("READS_PAST", "SHORT"):
            hits = [(t, "; ".join(w[:3])) for k, t, w in reached if k == kind]

            if hits:
                rows.append((kind, x, declared.get(name, 0), hits, []))

        at = bisect.bisect_left(addrs, x)

        if lost and at + 1 < len(addrs) and addrs[at + 1] <= x + size + 3 and moved(x, addrs[at + 1]):
            escapes.append(("ESCAPES", x, size, [(addrs[at + 1], "abuts it")], sorted(lost)[:4]))

    if args.at:
        want = int(args.at, 16)

        for start, i, kind, offset in uses.get(want, []):
            ins = img.code(start)[i]
            touched, notes = ([], set())

            if kind == "addr":
                touched, notes = reach_of(img, start, i, offset)
            span = ""

            if touched:
                span = "+0x%x..+0x%x" % (min(t[0] for t in touched), max(t[1] for t in touched))

            print("%-40s 0x%08x %-8s %-32s %s %s"
                  % (img.functions[start][1][:40], ins.address, kind,
                     "%s %s" % (ins.mnemonic, ins.op_str), span, "; ".join(sorted(notes))))
        return 0

    # BOUND: a walk that starts at one marked global and stops on the address of the next.
    for start in sorted(img.functions):
        if start in library:
            continue

        taken = []

        for i, ins in enumerate(img.code(start)):
            for op in ins.operands:
                value = None

                if op.type == x86.X86_OP_IMM and img.low <= op.imm < img.high:
                    value = op.imm
                elif op.type == x86.X86_OP_MEM and ins.mnemonic == "lea" and not op.mem.base                         and not op.mem.index and img.low <= op.mem.disp < img.high:
                    value = op.mem.disp

                if value is None or value not in byaddr:
                    continue

                if ins.mnemonic != "cmp":
                    taken.append(value)
                    continue

                before = [x for x in taken if x < value and sizes.get(byaddr[x])]

                if not before:
                    continue

                x = max(before)

                if x + sizes[byaddr[x]] < value - 3 or not moved(x, value):
                    continue

                rows.append(("BOUND", x, sizes[byaddr[x]],
                             [(value, "%s walks to it" % img.functions[start][1])], []))

    known = read_register()
    rows = [row for row in rows if (row[0], row[1]) not in known]
    shown = rows + (escapes if args.all else [])
    shown.sort(key=lambda r: (r[0], r[1]))

    for kind, x, size, hits, notes in shown:
        name = byaddr[x]
        print("%-10s 0x%08x %-34s size %d, here %+d" % (kind, x, name, size, ours[name] - x
                                                         if name in ours else 0))

        for target, why in hits:
            y = home_of(target)
            label = byaddr[y] if y is not None else "?"

            if y is not None and y != x and byaddr[y] in ours and name in ours:
                where = "here +0x%x" % ((ours[byaddr[y]] - ours[name] + (target - y)) & 0xffffffff)
            else:
                where = ""

            print("    +0x%-5x %-32s %-16s %s" % (target - x, label, where, why))

        for note in notes:
            print("    %s" % note)

    counts = collections.Counter(r[0] for r in rows)
    print("%d overlap, %d bound, %d reads past, %d short, %d unsized, %d escapes, %d registered (%s)"
          % (counts["OVERLAP"], counts["BOUND"], counts["READS_PAST"], counts["SHORT"],
             counts["UNSIZED"], len(escapes), len(known),
             os.path.relpath(args.map, ROOT)))

    return 1 if rows else 0


if __name__ == "__main__":
    sys.exit(main())
