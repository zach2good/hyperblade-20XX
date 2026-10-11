"""Compare each class's declared virtual count against the length of the table it names.

A class declaring fewer virtuals than its table has is incomplete, and is listed as a gap. A
class declaring more is a fault: its extra slots run past the end of the table. A table's length
is the distance to the next known table, as in tools/vtable_forest.py.

A class names its table by a `// VTABLE: HYPERX 0x004bxxxx` marker above it, or by a store of
`g_vtbl004bxxxx` in its own constructor or destructor.

An overrun can come from a constructor that stores its base's table, so the chain is measured
against the wrong one. Known overruns are registered in symbols/vtable_overruns.csv. The check
fails on an overrun not in the register, or a registered one that has closed.

    python tools/check_vtable_arity_declared.py            # the gaps, widest first
    python tools/check_vtable_arity_declared.py --over     # only the overruns
    python tools/check_vtable_arity_declared.py --save     # rewrite the register

Needs a copy of the original, so a fresh clone gets nothing and passes.
"""
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from peinfo import PEFile
from reference import reference_path

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")
ARITY = os.path.join(ROOT, "symbols", "vtablearity.h")
REGISTER = os.path.join(ROOT, "symbols", "vtable_overruns.csv")

TEXT_LOW = 0x00401000
TEXT_HIGH = 0x004a0000

DEFINED = re.compile(r"#define VTABLE_ARITY_(004b[0-9a-f]{4})")
WRITES = re.compile(r"^(\w+)::~?\1\([^)]*\)[^{]*\n\{(.*?)^\}", re.S | re.M)
VTABLE = re.compile(r"g_vtbl(004b[0-9a-f]{4})")
VIRTUAL = re.compile(r"^\s+virtual\s", re.M)
NAMED = re.compile(r"^\s+virtual\s[\w:*&~ ]*?(~?\w+)\s*\(", re.M)
MARKED = re.compile(r"^// VTABLE: HYPERX 0x(004b[0-9a-f]{4})\n(?:^//[^\n]*\n)*^class (\w+)", re.M)
CLASS = re.compile(r"^class (\w+)(?: : public (\w+))?\n\{\n(.*?)^\};", re.S | re.M)


def sourceNames(root):
    # Sources live in subdirectories, so this walks. Names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def sources():
    return [os.path.join(SRC, name) for name in sorted(sourceNames(SRC))
            if name.endswith((".cpp", ".h"))]


def text_of(path):
    return open(path, encoding="utf-8", errors="replace").read().replace("\r\n", "\n")


def boundaries():
    """Every address anything in src/ calls a vtable.

    Each one ends the table before it.
    """
    found = set()

    for path in sources():
        text = text_of(path)

        for hit in re.findall(r"VTABLE: HYPERX (0x004[0-9a-fA-F]{5})", text):
            found.add(int(hit, 16))

        for hit in re.findall(r"g_vtbl(004[bB][0-9a-fA-F]{4})", text):
            found.add(int(hit, 16))

    return sorted(found)


def table_lengths():
    pe = PEFile(reference_path())

    def read(address, count):
        for section in pe.sections:
            start = pe.image_base + section["vaddr"]
            if start <= address < start + section["rawsize"]:
                offset = section["rawoff"] + (address - start)
                room = (section["rawoff"] + section["rawsize"] - offset) // 4
                return list(struct.unpack_from("<%dI" % min(count, room), pe.data, offset))
        return []

    known = boundaries()
    lengths = {}

    for index, address in enumerate(known):
        limit = (known[index + 1] - address) // 4 if index + 1 < len(known) else 64
        slots = read(address, min(limit, 64))
        count = 0
        while count < len(slots) and TEXT_LOW <= slots[count] < TEXT_HIGH:
            count += 1
        if count:
            lengths[address] = count

    return lengths


def declared():
    """Return {class: [virtual count including bases, table, where]}."""
    whole = "\n".join(text_of(path) for path in sources())
    # A class declared in more than one file is measured by its fullest declaration.
    bodies = {}

    for match in CLASS.finditer(whole):
        name = match.group(1)
        candidate = (match.group(2), match.group(3))
        best = bodies.get(name)

        if best is None or len(VIRTUAL.findall(candidate[1])) > len(VIRTUAL.findall(best[1])):
            bodies[name] = candidate

    found = {}

    for path in sources():
        text = text_of(path)
        where = os.path.basename(path)

        for match in WRITES.finditer(text):
            table = VTABLE.search(match.group(2))
            if table:
                found.setdefault(match.group(1), [0, int(table.group(1), 16), where])

        for match in MARKED.finditer(text):
            found.setdefault(match.group(2), [0, int(match.group(1), 16), where])

    for klass in found:
        if klass not in bodies:
            continue

        base, body = bodies[klass]
        # A derived table is the base's length plus what the class ADDS. An override occupies
        # the base's slot rather than a new one, so a name already seen further down the chain
        # does not count again. Every destructor shares one slot, whatever its name.
        def slots(body):
            return set("~" if name.startswith("~") else name for name in NAMED.findall(body))

        names = slots(body)
        count = len(names)
        seen = {klass}

        while base and base in bodies and base not in seen:
            seen.add(base)
            base, body = bodies[base]
            for name in slots(body):
                if name not in names:
                    names.add(name)
                    count += 1

        found[klass][0] = count

    return found


def main():
    only_over = "--over" in sys.argv[1:]

    if reference_path(required=False) is None:
        print("no copy of the original is configured; skipping.")
        return 0

    lengths = table_lengths()
    rows = []

    for klass, (count, table, where) in declared().items():
        real = lengths.get(table)
        if real is not None:
            rows.append((real - count, klass, count, real, table, where))

    rows.sort()
    overruns = [row for row in rows if row[0] < 0]

    if "--save" in sys.argv[1:]:
        with open(REGISTER, "w", encoding="utf-8", newline="") as handle:
            handle.write("class,declared,table_slots,table" + chr(10))
            for gap, klass, count, real, table, where in sorted(overruns, key=lambda r: r[1]):
                handle.write("%s,%d,%d,%08x" % (klass, count, real, table) + chr(10))
        print("%d overrun(s) written to %s" % (len(overruns), REGISTER))
        return 0

    known = set()
    if os.path.exists(REGISTER):
        for line in open(REGISTER, encoding="utf-8").read().splitlines()[1:]:
            if line.strip():
                known.add(line.split(",")[0])

    fresh = [row for row in overruns if row[1] not in known]
    stale = known - {row[1] for row in overruns}

    for gap, klass, count, real, table, where in rows if only_over else reversed(rows):
        if only_over and gap >= 0:
            continue
        if gap == 0:
            continue
        print("%-28s %-24s declares %2d of %2d slots at 0x%08x%s"
              % (klass, where, count, real, table, "   OVERRUN" if gap < 0 else ""))

    for gap, klass, count, real, table, where in fresh:
        print("NEW OVERRUN  %s declares %d virtuals against %d slots at 0x%08x"
              % (klass, count, real, table))

    for klass in sorted(stale):
        print("CLOSED       %s no longer overruns; rerun with --save" % klass)

    print("")
    print("%d classes name a table, %d declare fewer virtuals than it has, "
          "%d declare more (%d new, %d closed)"
          % (len(rows), sum(1 for row in rows if row[0] > 0), len(overruns), len(fresh),
             len(stale)))

    return 1 if fresh or stale else 0


if __name__ == "__main__":
    raise SystemExit(main())
