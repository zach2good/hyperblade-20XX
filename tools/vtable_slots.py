"""Find vtable slots whose body the tree files under a class that cannot reach them.

A body declared as a plain member instead of an override still compiles, links and scores, but
the slot keeps the base's function and the body is never called.

This reads every marked table from the original image and names each slot from the tree's
FUNCTION markers. It reports a slot whose body the tree declares on a class that declares no
virtual at all: such a class emits no table, so no slot can reach the body.

A body on a base class is normal and is not reported. A slot with no FUNCTION marker is listed
but does not fail the run; it is usually an unmarked scalar-deleting destructor.

To diff one table slot by slot with reccmp:

    python -m reccmp.tools.asmcmp --target HYPERX --verbose 0x4b323c --no-color

    python tools/vtable_slots.py                   # every table with a suspect slot
    python tools/vtable_slots.py --table 0x4b3370  # one table, every slot
    python tools/vtable_slots.py --all             # do not hide slots that look right

This never reads our build, whose data sits at other addresses. Confirm a hit with reccmp.
Exit code 1 when any slot is suspect.
"""
import argparse
import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Slot values outside this run are not code and mean the table ended.
CODE_LOW = 0x00401000
CODE_HIGH = 0x004a0000

SLOT_MAX = 64

# A slot the original fills with one of these is a base's own no-op, shared by every table that
# does not override it. It says nothing about the table it appears in.
SHARED = ("NoOp", "_purecall", "GetStub", "Stub")
SHARED_NAMES = ("Object::Trace", "EngineObject::CheckObjectAI", "CAvoidObjects::IsReplaceable",
                "KeyConfigDialog::SaveKeysAs", "KeyConfigDialog::ClearKeys", "UIScreen::OnOtherButton")


def original_path():
    user = os.path.join(ROOT, "reccmp-user.yml")

    if not os.path.exists(user):
        sys.exit("reccmp-user.yml is missing; it says where the original lives")

    for line in open(user, encoding="utf-8"):
        match = re.search(r"path:\s*(\S+)", line)

        if match:
            return match.group(1)

    sys.exit("reccmp-user.yml names no path")


class Image:
    """A PE read far enough to turn a virtual address into a file offset."""

    def __init__(self, path):
        self.bytes = open(path, "rb").read()

        header = struct.unpack_from("<I", self.bytes, 0x3c)[0]
        sections = struct.unpack_from("<H", self.bytes, header + 6)[0]
        optional = struct.unpack_from("<H", self.bytes, header + 20)[0]

        self.base = struct.unpack_from("<I", self.bytes, header + 24 + 28)[0]
        self.sections = []

        at = header + 24 + optional

        for _ in range(sections):
            size, address, raw_size, raw = struct.unpack_from("<IIII", self.bytes, at + 8)

            self.sections.append((address, max(size, raw_size), raw))
            at += 40

    def offset(self, address):
        relative = address - self.base

        for start, size, raw in self.sections:
            if start <= relative < start + size:
                return raw + (relative - start)

        return None

    def slots(self, address, limit=SLOT_MAX):
        at = self.offset(address)

        if at is None:
            return []

        out = []

        for index in range(limit):
            value = struct.unpack_from("<I", self.bytes, at + index * 4)[0]

            if not CODE_LOW <= value <= CODE_HIGH:
                break

            out.append(value)

        return out


def source_files():
    for where, _, names in os.walk(os.path.join(ROOT, "src")):
        for name in names:
            if name.endswith((".cpp", ".h")):
                yield os.path.join(where, name)


def read_markers():
    """Every VTABLE and FUNCTION marker, by address, plus each class's declared bases."""
    tables = {}
    bodies = {}
    bases = {}
    virtuals = {}

    marker = re.compile(r"//\s*(VTABLE|FUNCTION):\s*HYPERX\s+0x([0-9a-fA-F]+)")
    declared = re.compile(r"^\s*(?:class|struct)\s+(\w+)\s*(?::\s*(.+?))?\s*$")
    defined = re.compile(r"([\w~]+)::([\w~]+)\s*\(")
    inherit = re.compile(r"(?:public|protected|private)?\s*(\w+)")

    for path in source_files():
        text = open(path, encoding="utf-8", errors="ignore").read()
        lines = text.splitlines()

        for hit in re.finditer(r"^\s*(?:class|struct)\s+(\w+)\s*:\s*([^{;]+)", text, re.M):
            bases[hit.group(1)] = [m for m in inherit.findall(hit.group(2)) if m not in
                                   ("public", "protected", "private", "virtual")]

        # Whether each class declares a virtual of its own. One that does not emits no table,
        # so a vtable slot can never reach a body declared on it.
        for hit in re.finditer(r"^\s*(?:class|struct)\s+(\w+)[^{;]*\{", text, re.M):
            at = text.index("{", hit.start())
            depth = 0

            for end in range(at, len(text)):
                if text[end] == "{":
                    depth += 1
                elif text[end] == "}":
                    depth -= 1

                    if depth == 0:
                        break

            virtuals[hit.group(1)] = "virtual" in text[at:end]

        for index, line in enumerate(lines):
            found = marker.search(line)

            if not found:
                continue

            kind = found.group(1)
            address = int(found.group(2), 16)

            # A marker binds to the next declaration, so walk past comment and blank lines.
            for ahead in lines[index + 1:index + 14]:
                stripped = ahead.strip()

                if not stripped or stripped.startswith("//"):
                    continue

                if kind == "VTABLE":
                    was = declared.match(ahead.rstrip())
                    tables[address] = (was.group(1) if was else stripped,
                                       os.path.relpath(path, ROOT))
                else:
                    was = defined.search(ahead)
                    bodies[address] = ((was.group(1), was.group(2)) if was else ("", stripped),
                                       os.path.relpath(path, ROOT))

                break

    return tables, bodies, bases, virtuals


def family(name, bases, seen=None):
    """A class and everything it derives from, as far as the headers say."""
    if seen is None:
        seen = set()

    if name in seen:
        return seen

    seen.add(name)

    for parent in bases.get(name, []):
        family(parent, bases, seen)

    return seen


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--table", help="one table, by address; every slot is listed")
    parser.add_argument("--all", action="store_true", help="do not hide slots that look right")
    args = parser.parse_args()

    original = Image(original_path())
    tables, bodies, bases, virtuals = read_markers()

    by_owner = {}

    for at, (name, _) in tables.items():
        by_owner.setdefault(name, at)

    wanted = int(args.table, 16) if args.table else None

    suspect = 0
    missing = 0
    checked = 0

    for address in sorted(tables):
        if wanted is not None and address != wanted:
            continue

        owner, where = tables[address]

        # Tables sit back to back, so the next marked table bounds this one.
        after = [other for other in sorted(tables) if other > address]
        limit = (after[0] - address) // 4 if after else SLOT_MAX

        slots = original.slots(address, min(limit, SLOT_MAX))

        if not slots:
            continue

        checked += 1
        rows = []

        for index, target in enumerate(slots):
            body = bodies.get(target)

            if body is None:
                rows.append((index, target, "", "not in the tree", True))
                continue

            (cls, method), file = body

            if any(mark in method or mark in cls for mark in SHARED) or "%s::%s" % (cls, method) in SHARED_NAMES:
                if args.all or wanted is not None:
                    rows.append((index, target, "%s::%s" % (cls, method), file, False))
                continue

            odd = False

            # Indices differ between a base's table and a derived one, so they are not compared.
            # A class that declares no virtual emits no table and cannot supply a slot.
            odd =cls != owner and virtuals.get(cls) is False

            if odd or args.all or wanted is not None:
                rows.append((index, target, "%s::%s" % (cls, method), file, odd))

        if not any(row[4] for row in rows) and wanted is None and not args.all:
            continue

        if not rows:
            continue

        print("%s  0x%08x  %s" % (owner, address, where))

        for index, target, name, file, odd in rows:
            print("    %s slot %2d  0x%08x  %-44s %s"
                  % ("!" if odd else " ", index, target, name or "-", file))

            if odd:
                if name:
                    suspect += 1
                else:
                    missing += 1

        print("")

    print("%d tables read, %d slots filed under a class that cannot reach them, "
          "%d with no body in the tree" % (checked, suspect, missing))

    return 1 if suspect else 0


if __name__ == "__main__":
    sys.exit(main())
