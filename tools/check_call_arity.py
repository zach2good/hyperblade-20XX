"""Check every written __cdecl signature against the argument count its callers push.

A missing trailing __cdecl parameter leaves the callee's bytes unchanged, so reccmp cannot see
it. A __cdecl caller cleans up with `add esp, N`, so N/4 is the argument count, read from the
original image.

Unusable readings are discarded:

  - VC6 merges the cleanup of adjacent calls. A site counts only when the pushes in front of
    the call account for all of N.
  - A cleanup folded into the epilogue leaves no `add esp`, so the site is skipped.

Every clean site of one callee must agree before anything is reported.

Bodies shorter than `STUB_BYTES` are skipped: the linker folds several unrelated functions onto
one bare `ret`, so no one signature fits.

Needs the original image named in reccmp-user.yml; without it the check skips and passes.

    python tools/check_call_arity.py                 report disagreements
    python tools/check_call_arity.py --all           also list the functions that agree
    python tools/check_call_arity.py --inventory     check symbols/functions.csv instead

Exit code 1 when a written signature disagrees with its call sites.
"""

import argparse
import bisect
import collections
import csv
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from peinfo import PEFile
from reference import reference_path

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INVENTORY = os.path.join(ROOT, "symbols", "functions.csv")
SOURCE_ROOT = os.path.join(ROOT, "src")

MARKER = re.compile(r"//\s*FUNCTION:\s*HYPERX\s+(0x[0-9a-fA-F]+)")

# Types wider than a dword occupy two argument slots.
WIDE = ("double", "long long", "__int64", "LARGE_INTEGER", "ULARGE_INTEGER")

# Below this a body is a bare `ret` that identical-code folding has given several callers
# with several signatures, and the call sites cannot name one of them.
STUB_BYTES = 8


def source_signatures():
    """Return {address: (path, line, declaration text)} for every FUNCTION marker."""
    found = {}
    for directory, _, names in os.walk(SOURCE_ROOT):
        for name in sorted(names):
            if not name.endswith((".cpp", ".c", ".h")):
                continue
            path = os.path.join(directory, name)
            with open(path, encoding="utf-8", errors="replace") as handle:
                lines = handle.read().splitlines()
            for index, line in enumerate(lines):
                match = MARKER.search(line)
                if not match:
                    continue
                declaration = gather(lines, index + 1)
                if declaration is not None:
                    found[int(match.group(1), 16)] = (path, index + 1, declaration)
    return found


def gather(lines, start):
    """Join the lines from `start` up to the opening brace or semicolon."""
    text = ""
    for line in lines[start:start + 8]:
        stripped = line.strip()
        if stripped.startswith("//") or not stripped:
            if text:
                break
            continue
        text += " " + stripped
        if "{" in stripped or ";" in stripped:
            return text.strip()
        if text.count("(") and text.count("(") == text.count(")"):
            return text.strip()
    return None


def parameters(declaration):
    """Return the parameter list of a declaration, or None when it cannot be read."""
    open_at = declaration.find("(")
    if open_at < 0:
        return None
    depth = 0
    close_at = -1
    for index in range(open_at, len(declaration)):
        if declaration[index] == "(":
            depth += 1
        elif declaration[index] == ")":
            depth -= 1
            if depth == 0:
                close_at = index
                break
    if close_at < 0:
        return None
    inner = declaration[open_at + 1:close_at].strip()
    if inner in ("", "void"):
        return []
    parts = []
    depth = 0
    current = ""
    for character in inner:
        if character in "(<[":
            depth += 1
        elif character in ")>]":
            depth -= 1
        if character == "," and depth == 0:
            parts.append(current.strip())
            current = ""
        else:
            current += character
    parts.append(current.strip())
    return parts


def stack_dwords(params):
    total = 0
    for param in params:
        if param == "...":
            return None
        if "*" in param or "&" in param:
            total += 1
            continue
        if any(wide in param for wide in WIDE):
            total += 2
            continue
        total += 1
    return total


RECORD = re.compile(r"^(?:struct|class)\s+(\w+)\s*(?::[^;{]*)?$", re.M)


def record_names():
    """Return every struct and class name the tree defines."""
    found = set()
    for directory, _, names in os.walk(SOURCE_ROOT):
        for name in names:
            if name.endswith(".h"):
                with open(os.path.join(directory, name), encoding="utf-8", errors="replace") as handle:
                    found.update(RECORD.findall(handle.read()))
    return found


def returns_record(declaration, records):
    """True when the function returns a struct by value, through a hidden pointer argument."""
    head = declaration.split("(")[0].replace("__cdecl", " ").replace("static", " ")
    words = head.split()
    return len(words) >= 2 and "*" not in head and "&" not in head and words[-2] in records


def is_cdecl_free_function(declaration):
    head = declaration.split("(")[0]
    # A namespace function and a static member are both spelled with ::, so only an explicit
    # __cdecl tells them apart from a __thiscall member.
    if "::" in head and "__cdecl" not in head:
        return False
    for other in ("__stdcall", "__fastcall", "__thiscall"):
        if other in head:
            return False
    return True


class Image:
    def __init__(self, path):
        self.pe = PEFile(path)
        section = next(s for s in self.pe.sections if s["name"] == ".text")
        self.base = self.pe.image_base + section["vaddr"]
        self.blob = self.pe.data[section["rawoff"]:section["rawoff"] + section["rawsize"]]

    def byte(self, address):
        return self.blob[address - self.base]

    def dword(self, address):
        return struct.unpack_from("<I", self.blob, address - self.base)[0]

    def holds(self, address, length=1):
        return self.base <= address and address + length <= self.base + len(self.blob)


def call_sites(image, bodies, wanted):
    """Return {callee: [(site, floor for a backwards walk)]} for every direct call rel32.

    The sweep covers all of .text, since some callers lie in code no inventory row covers. A
    stray 0xE8 inside an operand is filtered by requiring the target to be a function start.
    """
    covered = Ranges([(start, size) for start, size, _ in bodies])
    sites = collections.defaultdict(list)
    offset = image.base
    end = image.base + len(image.blob) - 5
    while offset < end:
        if image.byte(offset) == 0xE8:
            target = (offset + 5 + image.dword(offset + 1)) & 0xFFFFFFFF
            if target in wanted:
                owner = covered.find(offset)
                sites[target].append((offset, owner if owner is not None else offset - 64))
                offset += 5
                continue
        offset += 1
    return sites


class Ranges:
    """Address to enclosing function start over non-overlapping spans."""

    def __init__(self, spans):
        self.spans = sorted(spans)
        self.starts = [span[0] for span in self.spans]

    def find(self, address):
        index = bisect.bisect_right(self.starts, address) - 1
        if index < 0:
            return None
        start, size = self.spans[index]
        if address < start + size:
            return start
        return None


PUSH_IMM8 = 0x6A
PUSH_IMM32 = 0x68


def cleanup_after(image, site, limit):
    """The `add esp, N` that cleans up the call at `site`, or None.

    Reads forward at most `limit` bytes, giving up at anything that could be another
    call, a branch, or a stack move of its own.
    """
    offset = site + 5
    stop = site + 5 + limit
    while offset < stop and image.holds(offset, 3):
        first = image.byte(offset)
        if first == 0x83 and image.byte(offset + 1) == 0xC4:
            return image.byte(offset + 2)
        if first == 0x81 and image.byte(offset + 1) == 0xC4:
            return image.dword(offset + 2)
        if first in (0xE8, 0xE9, 0xEB, 0xC3, 0xC2, 0xFF, 0x0F) or 0x70 <= first <= 0x7F:
            return None
        offset += 1
    return None


def pushes_before(image, site, start):
    """Count the argument pushes standing in front of a call, walking backwards.

    Backwards decoding is unreliable in general, so this only recognises the forms VC6
    uses to set an argument up: a bare `push reg`, `push imm8`, `push imm32`, and the
    `mov reg, [...]` / `lea reg, [...]` that loads one. It stops at anything else, which
    makes the count a floor, which is enough to reject a cleanup that belongs to two calls.
    """
    count = 0
    offset = site - 1
    while offset >= start:
        byte = image.byte(offset)
        if 0x50 <= byte <= 0x57:
            count += 1
            offset -= 1
            continue
        if byte == PUSH_IMM8 and offset >= start + 1:
            count += 1
            offset -= 2
            continue
        if offset >= start + 4 and image.byte(offset - 4) == PUSH_IMM32:
            count += 1
            offset -= 5
            continue
        # An argument load sits between pushes often enough that stopping at one
        # would floor almost every site at zero. Skip the short forms only.
        skipped = False
        for length in (2, 3, 4, 6, 7):
            if offset - length + 1 < start:
                continue
            head = image.byte(offset - length + 1)
            if head in (0x8B, 0x8D, 0xA1, 0x33, 0x0F):
                offset -= length
                skipped = True
                break
        if not skipped:
            break
    return count


def inventory_signatures(rows):
    """Return {address: (source, row, declaration)} for the __cdecl rows of functions.csv.

    The inventory mirrors Ghidra, so this checks the database rather than the source, and it
    covers every row, written or not.
    """
    found = {}
    for address, row in rows.items():
        if row["convention"] != "__cdecl":
            continue
        found[address] = ("symbols/functions.csv", 0, row["signature"])
    return found


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--all", action="store_true", help="list the agreements too")
    parser.add_argument("--inventory", action="store_true",
                        help="check symbols/functions.csv instead of the written source")
    args = parser.parse_args()

    # A fresh clone has no copy of the original, so this skips and passes rather than fail.
    original = reference_path(required=False)

    if original is None:
        print("no copy of the original is configured, so the callers cannot be read.")
        print("Name yours in reccmp-user.yml and run this again. Skipping.")
        return 0

    image = Image(original)

    bodies = []
    names = {}
    sizes = {}
    rows = {}
    with open(INVENTORY, encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            if not str(row["size"]).isdigit():
                continue
            address = int(row["address"], 16)
            bodies.append((address, int(row["size"]), row["name"]))
            names[address] = row["name"]
            sizes[address] = int(row["size"])
            rows[address] = row

    if args.inventory:
        written = inventory_signatures(rows)
    else:
        written = source_signatures()
    sites = call_sites(image, bodies, set(written))

    records = record_names()
    disagreements = []
    agreements = 0
    for address, (path, line, declaration) in sorted(written.items()):
        if address not in sites:
            continue
        if sizes.get(address, 0) < STUB_BYTES:
            continue
        if not is_cdecl_free_function(declaration):
            continue
        params = parameters(declaration)
        if params is None:
            continue
        declared = stack_dwords(params)
        if declared is None:
            continue
        if returns_record(declaration, records):
            declared += 1

        readings = set()
        for site, start in sites[address]:
            cleanup = cleanup_after(image, site, 24)
            if cleanup is None or cleanup % 4 or cleanup > 64:
                continue
            if pushes_before(image, site, start) < cleanup // 4:
                continue
            readings.add(cleanup // 4)

        if len(readings) != 1:
            continue
        actual = readings.pop()
        if actual == declared:
            agreements += 1
            if args.all:
                print("agree     %s  0x%08X  %d" % (names.get(address, "?"), address, actual))
            continue
        disagreements.append((address, names.get(address, "?"), declared, actual, path, line))

    for address, name, declared, actual, path, line in disagreements:
        print("0x%08X  %-46s declared %d, callers push %d" % (address, name, declared, actual))
        if line:
            print("            %s:%d" % (os.path.relpath(path, ROOT).replace("\\", "/"), line))

    subject = "inventory" if args.inventory else "written"
    print("call_arity: %d %s __cdecl functions read from their call sites, "
          "%d agree, %d disagree" % (agreements + len(disagreements), subject, agreements,
                                     len(disagreements)))
    return 1 if disagreements else 0


if __name__ == "__main__":
    sys.exit(main())
