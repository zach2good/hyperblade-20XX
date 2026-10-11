"""Read the reference image's sections, and the vtable addresses this tree names.

A library for the tools that walk the image's tables: `Image` reads words out of .rdata and
finds the `ret N` of a slot's body, and `knownTables` lists every address src/ marks as a vtable.
"""
import importlib
import os
import re
import struct

import reference

capstone = importlib.import_module("capstone")

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def sourceFiles(suffixes):
    """Every source file under src/, at any depth."""
    for where, _, names in os.walk(os.path.join(ROOT, "src")):
        for name in sorted(names):
            if name.endswith(suffixes):
                yield os.path.join(where, name)


def referencePath():
    return reference.reference_path()


class Image:
    def __init__(self, path):
        self.data = open(path, "rb").read()

        pe = struct.unpack_from("<I", self.data, 0x3c)[0]
        count = struct.unpack_from("<H", self.data, pe + 6)[0]
        optional = struct.unpack_from("<H", self.data, pe + 20)[0]

        self.base = struct.unpack_from("<I", self.data, pe + 24 + 28)[0]
        self.sections = []

        for i in range(count):
            at = pe + 24 + optional + i * 40
            name = self.data[at:at + 8].rstrip(b"\0").decode("ascii", "replace")
            self.sections.append((name,
                                  self.base + struct.unpack_from("<I", self.data, at + 12)[0],
                                  struct.unpack_from("<I", self.data, at + 8)[0],
                                  struct.unpack_from("<I", self.data, at + 20)[0]))

        self.text = [s for s in self.sections if s[0] == ".text"][0]
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

    def offset(self, address):
        for unused, start, size, raw in self.sections:
            if start <= address < start + size:
                return raw + (address - start)

        return None

    def isCode(self, address):
        return self.text[1] <= address < self.text[1] + self.text[2]

    def word(self, address):
        at = self.offset(address)

        if at is None:
            return None

        return struct.unpack_from("<I", self.data, at)[0]

    def pops(self, address):
        """The `ret N` of the slot's body, following a thunk to the body it jumps to.

        Only a `jmp` at the body's first instruction is a thunk (CDDTextSurface::Draw at
        0x00437850). A later jump, such as a jump table, is ordinary control flow and the scan
        runs past it.
        """
        for unused in range(8):
            at = self.offset(address)

            if at is None:
                return None

            target = None

            for instruction in self.md.disasm(self.data[at:at + 0x1000], address):
                if instruction.mnemonic == "ret":
                    if instruction.op_str:
                        return int(instruction.op_str, 16)

                    return 0

                if instruction.mnemonic == "jmp" and instruction.address == address:
                    try:
                        target = int(instruction.op_str, 16)
                    except ValueError:
                        target = None

                    break

            if target is None or not self.isCode(target):
                return None

            address = target

        return None


def knownTables():
    found = set()

    for path in sourceFiles((".h", ".cpp")):
        text = open(path, encoding="utf-8", errors="replace").read()

        for hit in re.findall(r"VTABLE: HYPERX (0x004[0-9a-fA-F]{5})", text):
            found.add(int(hit, 16))

        for hit in re.findall(r"g_vtbl(004[bB][0-9a-fA-F]{4})", text):
            found.add(int("0x" + hit, 16))

    return sorted(found)
