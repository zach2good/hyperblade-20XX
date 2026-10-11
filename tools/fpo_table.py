"""Read the FPO function table out of HYPERX.EXE v1.0.0.0.

v1.0.0.0 was linked without stripping its debug directory. The FPO entry there is an
array of FPO_DATA records, one per frame-pointer-omitted function, each giving the
function's start, its exact byte size, its prologue length, and how many dwords of
parameters and locals its frame holds.

v1.0.0.1, which this project actually matches, has an empty debug directory. The two
images were built by different compilers, so an address from this table means nothing
in v1.0.0.1. Use it as a second reading: sizes, boundaries and parameter counts to
check a v1.0.0.1 decode against, by way of the string anchors in fpo_anchors.py.

Usage:
    python tools/fpo_table.py <hyperx-v1.0.0.0.exe> [-o symbols/v1000_fpo.csv]
"""

import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from peinfo import PEFile

DEBUG_TYPE_CODEVIEW = 2
DEBUG_TYPE_FPO = 3
DEBUG_TYPE_MISC = 4

FRAME_NAMES = {0: "fpo", 1: "trap", 2: "tss", 3: "nonfpo"}


def debug_entries(pe):
    """Yield (type, size, file offset) for each image debug directory entry."""
    rva, size = struct.unpack_from("<II", pe.data, pe.optional + 96 + 6 * 8)
    if not rva or not size:
        return
    offset = rva_to_offset(pe, rva)
    if offset is None:
        return
    for index in range(size // 28):
        entry = offset + index * 28
        kind = struct.unpack_from("<I", pe.data, entry + 12)[0]
        data_size = struct.unpack_from("<I", pe.data, entry + 16)[0]
        data_offset = struct.unpack_from("<I", pe.data, entry + 24)[0]
        yield kind, data_size, data_offset


def rva_to_offset(pe, rva):
    for section in pe.sections:
        if section["vaddr"] <= rva < section["vaddr"] + max(section["vsize"], section["rawsize"]):
            return section["rawoff"] + rva - section["vaddr"]
    return None


def pdb_path(pe):
    """Return the PDB path from the CODEVIEW entry, or None."""
    for kind, size, offset in debug_entries(pe):
        if kind != DEBUG_TYPE_CODEVIEW:
            continue
        blob = pe.data[offset:offset + size]
        if blob[:4] != b"NB10":
            return None
        return blob[16:].rstrip(b"\0").decode("latin1")
    return None


def fpo_records(pe):
    """Return [(rva, size, locals, params, prolog, regs, seh, bp, frame), ...] by address."""
    for kind, size, offset in debug_entries(pe):
        if kind != DEBUG_TYPE_FPO:
            continue
        blob = pe.data[offset:offset + size]
        out = []
        for index in range(len(blob) // 16):
            start, code, local, param, flags = struct.unpack_from("<IIIHH", blob, index * 16)
            out.append((start, code, local, param,
                        flags & 0xFF, (flags >> 8) & 7,
                        (flags >> 11) & 1, (flags >> 12) & 1, (flags >> 14) & 3))
        out.sort()
        return out
    return []


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", help="path to HYPERX.EXE v1.0.0.0")
    parser.add_argument("-o", "--output", help="write CSV here instead of stdout")
    args = parser.parse_args()

    if not os.path.isfile(args.image):
        sys.exit("fpo_table: no such file: %s" % args.image)

    pe = PEFile(args.image)
    records = fpo_records(pe)
    if not records:
        sys.exit("fpo_table: %s has no FPO debug entry. v1.0.0.1 has none; you want v1.0.0.0."
                 % args.image)

    text = next((s for s in pe.sections if s["name"] == ".text"), None)
    lines = ["address,size,prolog,saved_regs,param_dwords,local_dwords,has_seh,uses_bp,frame"]
    covered = 0
    overlaps = 0
    previous_end = 0
    for start, code, local, param, prolog, regs, seh, bp, frame in records:
        address = pe.image_base + start
        if start < previous_end:
            overlaps += 1
        previous_end = start + code
        covered += code
        lines.append("0x%08X,%d,%d,%d,%d,%d,%d,%d,%s"
                     % (address, code, prolog, regs, param, local, seh, bp,
                        FRAME_NAMES.get(frame, str(frame))))

    body = "\n".join(lines) + "\n"
    if args.output:
        with open(args.output, "w", newline="\n") as handle:
            handle.write(body)
    else:
        sys.stdout.write(body)

    share = 100.0 * covered / text["vsize"] if text else 0.0
    sys.stderr.write("fpo_table: %d records, %d bytes covered of %d in .text (%.1f%%), %d overlaps\n"
                     % (len(records), covered, text["vsize"] if text else 0, share, overlaps))
    path = pdb_path(pe)
    if path:
        sys.stderr.write("fpo_table: built as %s\n" % path)


if __name__ == "__main__":
    main()
