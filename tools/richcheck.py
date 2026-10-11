"""Report the Rich header of a built image and compare its toolchain against the original.

The Rich header records which compiler build produced every object the linker consumed.
Matching it is a precondition for a byte-accurate recompilation: the wrong back end can
still produce a working executable, but never an identical one.

    python tools/richcheck.py build/out/HYPERX.exe --reference .../HYPERX.EXE

Exits non-zero when a toolchain component in the build does not appear in the original.
"""

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from peinfo import PEFile, product_name


def show(path):
    image = PEFile(path)
    entries = image.rich_header()
    print("%s" % path)
    print("  linker %d.%d, subsystem %d, image base %#x, file alignment %#x"
          % (image.linker_major, image.linker_minor, image.subsystem,
             image.image_base, image.file_alignment))
    if entries is None:
        print("  no Rich header")
        return image, {}
    stamps = {}
    for prodid, build, count in sorted(entries, key=lambda row: -row[2]):
        stamps[(prodid, build)] = count
        print("  %08x  %-42s x%d" % ((prodid << 16) | build, product_name(prodid, build), count))
    return image, stamps


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image")
    parser.add_argument("--reference")
    args = parser.parse_args()

    _, built = show(args.image)
    if not args.reference:
        return 0

    if not os.path.exists(args.reference):
        print("\nreference not found: %s" % args.reference)
        print("Set HYPERX_REFERENCE in the CMake cache to the original executable.")
        return 0

    print()
    _, original = show(args.reference)

    print("\ntoolchain comparison")
    missing = []
    for key in sorted(built):
        prodid, build = key
        mark = "ok     " if key in original else "ABSENT "
        if key not in original:
            missing.append(key)
        print("  %s %08x  %s" % (mark, (prodid << 16) | build, product_name(prodid, build)))

    for key in sorted(original):
        if key not in built:
            prodid, build = key
            print("  unused  %08x  %s" % ((prodid << 16) | build, product_name(prodid, build)))

    if missing:
        print("\nFAIL: %d component(s) in this build appear nowhere in the original."
              % len(missing))
        print("The build is using a toolchain the original never used.")
        return 1

    print("\nOK: every component used by this build is one the original also used.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
