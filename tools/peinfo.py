"""A minimal PE reader for tools/extract_resources.py, which every link runs.

Deliberately dependency free, so it works with a bare Python install.
"""

import struct

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
