"""Patch the COFF TimeDateStamp of a built image to the original's link time.

LINK.EXE 6.00.8447 has no way to set the stamp: /TIMESTAMP, /DATE and /STAMP are rejected, and
/Brepro writes 0xffffffff. So this rewrites the four bytes at PE+8 in place. The image has no PE
checksum, as the original has none, so nothing else needs recomputing.

Idempotent, so it is safe as a POST_BUILD step. Do not run it on build/out/HYPERX_sym.exe: that
/DEBUG image is not the one compared, and its debug directory timestamp is tied to the header.

    python tools/stamp.py build/out/HYPERX.exe
    python tools/stamp.py build/out/HYPERX.exe --reference .../HYPERX.EXE
    python tools/stamp.py build/out/HYPERX.exe --check
"""

import argparse
import datetime
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from peinfo import PEFile

# 2000-06-26 22:19:11 UTC, the link time of HYPERX.EXE v1.0.0.1.
ORIGINAL_TIMESTAMP = 0x3957D6DF


def describe(stamp):
    if stamp == 0xFFFFFFFF:
        return "%#010x (reproducible marker)" % stamp
    moment = datetime.datetime.fromtimestamp(stamp, datetime.UTC)
    return "%#010x (%s UTC)" % (stamp, moment.strftime("%Y-%m-%d %H:%M:%S"))


def stamp_offset(path):
    """File offset of IMAGE_FILE_HEADER.TimeDateStamp, and the value there."""
    image = PEFile(path)
    return image.pe_offset + 8, image.timestamp


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image")
    parser.add_argument("--stamp", help="value to write, decimal or 0x hex")
    parser.add_argument("--reference", help="read the value out of this image instead")
    parser.add_argument("--check", action="store_true",
                        help="report only, exit non-zero when the stamp is wrong")
    args = parser.parse_args()

    wanted = ORIGINAL_TIMESTAMP
    if args.reference:
        if not os.path.exists(args.reference):
            print("reference not found: %s" % args.reference)
            print("Falling back to the recorded value %s." % describe(wanted))
        else:
            wanted = PEFile(args.reference).timestamp
    if args.stamp:
        wanted = int(args.stamp, 0)

    if os.path.basename(args.image).lower() == "hyperx_sym.exe":
        print("refusing to stamp %s: the /DEBUG image is not the one under comparison"
              % args.image)
        return 1

    offset, current = stamp_offset(args.image)

    if current == wanted:
        print("%s: timestamp already correct, %s" % (args.image, describe(current)))
        return 0

    if args.check:
        print("%s: timestamp is %s, wanted %s" % (args.image, describe(current), describe(wanted)))
        return 1

    with open(args.image, "r+b") as handle:
        handle.seek(offset)
        handle.write(struct.pack("<I", wanted))

    print("%s: timestamp patched at %#x, %s to %s"
          % (args.image, offset, describe(current), describe(wanted)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
