"""Minimal PE, COFF and archive readers shared by the verification tools.

Deliberately dependency free. These run on every build, so they must work with a bare
Python install and nothing else.
"""

import struct

COMP_ID_NAMES = {
    0x0000: "unmarked",
    0x0001: "import descriptors",
    0x0006: "cvtres",
    0x000A: "C back end",
    0x000B: "C++ back end",
    0x000C: "AliasObj (OLDNAMES.LIB)",
    0x000E: "MASM",
    0x0013: "linker 5.12 (import libraries)",
}


def product_name(prodid, build):
    base = COMP_ID_NAMES.get(prodid, "prodid %#06x" % prodid)
    if prodid in (0x000A, 0x000B):
        known = {8168: "VC6 RTM", 8447: "VC6 SP3", 8799: "VC6 SP4", 8966: "VC6 SP5", 9782: "VC6 SP6"}
        if build in known:
            return "%s 12.00.%d (%s)" % (base, build, known[build])
        return "%s 12.00.%d" % (base, build)
    if build:
        return "%s build %d" % (base, build)
    return base


class PEFile:
    def __init__(self, path):
        self.path = path
        with open(path, "rb") as handle:
            self.data = handle.read()
        data = self.data
        if data[:2] != b"MZ":
            raise ValueError("%s is not a PE image" % path)
        self.pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
        if data[self.pe_offset:self.pe_offset + 4] != b"PE\0\0":
            raise ValueError("%s has no PE signature" % path)
        head = self.pe_offset + 4
        self.machine, self.section_count, self.timestamp = struct.unpack_from("<HHI", data, head)
        self.optional_size = struct.unpack_from("<H", data, head + 16)[0]
        self.characteristics = struct.unpack_from("<H", data, head + 18)[0]
        self.optional = self.pe_offset + 24
        self.linker_major = data[self.optional + 2]
        self.linker_minor = data[self.optional + 3]
        self.image_base = struct.unpack_from("<I", data, self.optional + 28)[0]
        self.section_alignment = struct.unpack_from("<I", data, self.optional + 32)[0]
        self.file_alignment = struct.unpack_from("<I", data, self.optional + 36)[0]
        self.subsystem = struct.unpack_from("<H", data, self.optional + 68)[0]
        self.sections = []
        for index in range(self.section_count):
            entry = self.optional + self.optional_size + index * 40
            name = data[entry:entry + 8].rstrip(b"\0").decode("latin1")
            vsize, vaddr, rawsize, rawoff = struct.unpack_from("<IIII", data, entry + 8)
            flags = struct.unpack_from("<I", data, entry + 36)[0]
            self.sections.append(
                {"name": name, "vsize": vsize, "vaddr": vaddr,
                 "rawsize": rawsize, "rawoff": rawoff, "flags": flags})

    def rich_header(self):
        """Return [(prodid, build, count), ...] or None if the image has no Rich header."""
        head = self.data[:self.pe_offset]
        marker = head.find(b"Rich")
        if marker < 0:
            return None
        key = struct.unpack_from("<I", head, marker + 4)[0]
        start = None
        for offset in range(0x40, marker, 4):
            if struct.unpack_from("<I", head, offset)[0] ^ key == 0x536E6144:
                start = offset
                break
        if start is None:
            return None
        entries = []
        offset = start + 16
        while offset < marker:
            comp = struct.unpack_from("<I", head, offset)[0] ^ key
            count = struct.unpack_from("<I", head, offset + 4)[0] ^ key
            entries.append((comp >> 16, comp & 0xFFFF, count))
            offset += 8
        return entries


def archive_members(data):
    """Yield (name, bytes) for each real member of a COFF archive, or the whole blob."""
    if data[:8] != b"!<arch>\n":
        yield "<object>", data
        return
    position = 8
    while position + 60 <= len(data):
        name = data[position:position + 16].decode("latin1").strip()
        size = int(data[position + 48:position + 58].decode("latin1").strip() or 0)
        if name not in ("/", "//"):
            yield name, data[position + 60:position + 60 + size]
        position += 60 + size + (size & 1)


def object_comp_id(obj):
    """Read the @comp.id absolute symbol out of a COFF object, or None."""
    if len(obj) < 20:
        return None
    machine, section_count, timestamp, symbol_table, symbol_count = struct.unpack_from("<HHIII", obj, 0)
    if machine != 0x14C or not symbol_table or not symbol_count:
        return None
    if symbol_table + symbol_count * 18 > len(obj):
        return None
    strings = symbol_table + symbol_count * 18
    index = 0
    while index < symbol_count:
        entry = symbol_table + index * 18
        record = obj[entry:entry + 18]
        if record[:4] == b"\0\0\0\0":
            offset = struct.unpack_from("<I", record, 4)[0]
            end = obj.find(b"\0", strings + offset)
            name = obj[strings + offset:end]
        else:
            name = record[:8].rstrip(b"\0")
        if name == b"@comp.id":
            return struct.unpack_from("<I", record, 8)[0]
        index += 1 + record[17]
    return None
