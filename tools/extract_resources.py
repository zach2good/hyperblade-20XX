"""Read the resource directory out of the user's own copy of the original and write a .res.

The repository carries none of the original's resources, but the link needs them. The build
reads them from the user's own copy of the game into build/. Nothing produced here is committed.

A .res carries the leaf bytes verbatim, where a .rc would have RC.EXE rebuild every dialog,
string table and icon group from text. Type 240 is a private type with no RC statement.

    python tools/extract_resources.py --output build/generated/HYPERX.res
    python tools/extract_resources.py <original.exe> --output <path>
"""

import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from peinfo import PEFile
from reference import reference_path

# The PE resource directory does not record these. RC.EXE writes the same values for every
# resource type this program uses, so they are stated once.
MEMORY_FLAGS = 0x1030
DATA_VERSION = 0
VERSION = 0
CHARACTERISTICS = 0


class Leaf:
    def __init__(self, type_id, name_id, language, offset, data):
        self.type_id = type_id
        self.name_id = name_id
        self.language = language
        self.offset = offset
        self.data = data


def read_directory(blob, offset, path, leaves):
    named, ids = struct.unpack_from("<HH", blob, offset + 12)
    entry = offset + 16

    for _ in range(named + ids):
        key, child = struct.unpack_from("<II", blob, entry)
        entry += 8

        if key & 0x80000000:
            at = key & 0x7FFFFFFF
            length = struct.unpack_from("<H", blob, at)[0]
            key = blob[at + 2:at + 2 + length * 2].decode("utf-16-le")

        path.append(key)

        if child & 0x80000000:
            read_directory(blob, child & 0x7FFFFFFF, path, leaves)
        else:
            rva, size = struct.unpack_from("<II", blob, child)[:2]
            at = rva - path[0]
            leaves.append(Leaf(path[1], path[2], path[3], at, blob[at:at + size]))

        path.pop()


def resource_leaves(path):
    image = PEFile(path)
    section = None

    for candidate in image.sections:
        if candidate["name"] == ".rsrc":
            section = candidate

    if not section:
        raise SystemExit("%s has no .rsrc section" % path)

    blob = image.data[section["rawoff"]:section["rawoff"] + section["rawsize"]]
    leaves = []
    read_directory(blob, 0, [section["vaddr"]], leaves)

    return sorted(leaves, key=lambda leaf: leaf.offset)


def encode_key(key):
    if isinstance(key, str):
        return key.encode("utf-16-le") + b"\0\0"

    return struct.pack("<HH", 0xFFFF, key)


def encode_leaf(leaf):
    head = encode_key(leaf.type_id) + encode_key(leaf.name_id)
    head += b"\0" * (-len(head) & 3)
    head += struct.pack("<IHHII", DATA_VERSION, MEMORY_FLAGS, leaf.language,
                        VERSION, CHARACTERISTICS)

    record = struct.pack("<II", len(leaf.data), len(head) + 8) + head + leaf.data

    return record + b"\0" * (-len(record) & 3)


def encode(leaves):
    # An empty first record marks the file as the 32-bit .res format, which CVTRES requires.
    out = struct.pack("<IIHHHHIHHII", 0, 32, 0xFFFF, 0, 0xFFFF, 0, 0, 0, 0, 0, 0)

    for leaf in leaves:
        out += encode_leaf(leaf)

    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original", nargs="?", help="the original executable to read")
    parser.add_argument("--output", required=True, help="the .res file to write")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()

    original = args.original or reference_path()
    leaves = resource_leaves(original)

    directory = os.path.dirname(os.path.abspath(args.output))

    if directory:
        os.makedirs(directory, exist_ok=True)

    blob = encode(leaves)

    with open(args.output, "wb") as handle:
        handle.write(blob)

    if not args.quiet:
        types = len(set(leaf.type_id for leaf in leaves))
        print("%d resources of %d types, %d bytes -> %s"
              % (len(leaves), types, len(blob), args.output))


if __name__ == "__main__":
    main()
