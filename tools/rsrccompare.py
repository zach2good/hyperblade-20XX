"""Compare the built .rsrc section against the original's, byte for byte.

The only thing in .rsrc that depends on where the section lands is the OffsetToData field
of each IMAGE_RESOURCE_DATA_ENTRY, which is an image-relative address. Every other byte,
the directory tree, the name strings, the alignment padding and the leaf data, is position
independent. So this reports two figures:

  raw        the bytes as linked, which cannot agree until .text, .rdata and .data reach
             the original's sizes and .rsrc therefore lands at the original's address
  rebased    the same bytes with each OffsetToData reduced to an offset within the
             section, which is the part the resource extraction is responsible for

`rebased: identical` is the statement that the extracted resources are right. `raw` turns
into `identical` on its own once the rest of the image is the right size.

    python tools/rsrccompare.py build/out/HYPERX.exe <original>
"""

import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from peinfo import PEFile
from reference import reference_path


def section(image):
    for candidate in image.sections:
        if candidate["name"] == ".rsrc":
            return candidate

    return None


def data_entry_offsets(blob, at, found):
    named, ids = struct.unpack_from("<HH", blob, at + 12)
    entry = at + 16

    for _ in range(named + ids):
        key, child = struct.unpack_from("<II", blob, entry)
        entry += 8

        if child & 0x80000000:
            data_entry_offsets(blob, child & 0x7FFFFFFF, found)
        else:
            found.append(child)


def rebase(blob, vaddr):
    out = bytearray(blob)
    found = []
    data_entry_offsets(out, 0, found)

    for at in found:
        rva = struct.unpack_from("<I", out, at)[0]
        struct.pack_into("<I", out, at, rva - vaddr)

    return bytes(out), len(found)


def read(path):
    image = PEFile(path)
    part = section(image)

    if not part:
        return None, None, None

    blob = image.data[part["rawoff"]:part["rawoff"] + part["vsize"]]

    return blob, part["vaddr"], part["rawsize"]


def report(name, left, right):
    if len(left) != len(right):
        print("  %-8s different sizes: %#x against %#x" % (name, len(left), len(right)))
        return False

    differing = sum(1 for a, b in zip(left, right) if a != b)

    if not differing:
        print("  %-8s identical, %#x bytes" % (name, len(left)))
        return True

    first = next(i for i, (a, b) in enumerate(zip(left, right)) if a != b)
    print("  %-8s %d of %d bytes differ, first at +%#x (built %02x, original %02x)"
          % (name, differing, len(left), first, left[first], right[first]))

    return False


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("built")
    parser.add_argument("original", nargs="?")
    args = parser.parse_args()

    original = args.original or reference_path()

    built_blob, built_vaddr, built_raw = read(args.built)
    original_blob, original_vaddr, original_raw = read(original)

    if original_blob is None:
        raise SystemExit("%s has no .rsrc section" % original)

    if built_blob is None:
        print("%s has no .rsrc section; the resource step did not run" % args.built)
        raise SystemExit(1)

    print(".rsrc  built at %#x (%#x raw), original at %#x (%#x raw)"
          % (built_vaddr, built_raw, original_vaddr, original_raw))

    raw_ok = report("raw", built_blob, original_blob)

    built_flat, count = rebase(built_blob, built_vaddr)
    original_flat, _ = rebase(original_blob, original_vaddr)

    rebased_ok = report("rebased", built_flat, original_flat)

    print("  %d resource data entries rebased" % count)

    if rebased_ok and not raw_ok:
        print("\nThe resources are right. The addresses in them follow once .text, .rdata\n"
              "and .data reach the original's sizes.")

    if not rebased_ok:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
