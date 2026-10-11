"""Compare a built image against the original and report the first difference.

Reports header fields, then the section table, then the first differing byte in each
section that both images have.

    python tools/pecompare.py build/out/HYPERX.exe .../HYPERX.EXE

Exits non-zero only when the COFF timestamp differs. Other fields may differ while the sources
are incomplete. The timestamp may not: `tools/stamp.py` patches it after every link, so a
mismatch means that step did not run.
"""

import argparse
import datetime
import hashlib
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from peinfo import PEFile


def timestamp(image):
    stamp = image.timestamp
    if stamp == 0xFFFFFFFF:
        return "%#010x (reproducible)" % stamp
    moment = datetime.datetime.fromtimestamp(stamp, datetime.UTC)
    return "%#010x (%s)" % (stamp, moment.strftime("%Y-%m-%d %H:%M"))


def field_table(built, original):
    rows = [
        ("size", len(built.data), len(original.data)),
        ("machine", hex(built.machine), hex(original.machine)),
        ("timestamp", timestamp(built), timestamp(original)),
        ("sections", built.section_count, original.section_count),
        ("linker", "%d.%d" % (built.linker_major, built.linker_minor),
         "%d.%d" % (original.linker_major, original.linker_minor)),
        ("image base", hex(built.image_base), hex(original.image_base)),
        ("section align", hex(built.section_alignment), hex(original.section_alignment)),
        ("file align", hex(built.file_alignment), hex(original.file_alignment)),
        ("subsystem", built.subsystem, original.subsystem),
        ("characteristics", hex(built.characteristics), hex(original.characteristics)),
    ]
    print("%-16s %-24s %-24s" % ("field", "built", "original"))
    for name, left, right in rows:
        mark = " " if str(left) == str(right) else "*"
        print("%s%-15s %-24s %-24s" % (mark, name, left, right))


def section_table(built, original):
    by_name = {section["name"]: section for section in original.sections}
    print("\n%-10s %-30s %-30s" % ("section", "built vsize/rawsize", "original vsize/rawsize"))
    for section in built.sections:
        other = by_name.get(section["name"])
        left = "%#x / %#x" % (section["vsize"], section["rawsize"])
        right = "absent"
        if other:
            right = "%#x / %#x" % (other["vsize"], other["rawsize"])
        mark = " " if other and left == right else "*"
        print("%s%-9s %-30s %-30s" % (mark, section["name"], left, right))
    for name in by_name:
        if not any(section["name"] == name for section in built.sections):
            print("*%-9s %-30s %-30s" % (name, "absent", "present"))


def first_difference(built, original):
    print("\nfirst differing byte per shared section")
    by_name = {section["name"]: section for section in original.sections}
    identical = True
    for section in built.sections:
        other = by_name.get(section["name"])
        if not other:
            continue
        span = min(section["rawsize"], other["rawsize"])
        left = built.data[section["rawoff"]:section["rawoff"] + span]
        right = original.data[other["rawoff"]:other["rawoff"] + span]
        if left == right and section["rawsize"] == other["rawsize"]:
            print("  %-9s identical (%#x bytes)" % (section["name"], span))
            continue
        identical = False
        offset = next((i for i in range(span) if left[i] != right[i]), span)
        print("  %-9s differs at +%#x (file %#x), built %02x original %02x"
              % (section["name"], offset, section["rawoff"] + offset,
                 left[offset] if offset < span else 0,
                 right[offset] if offset < span else 0))
    return identical


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("built")
    parser.add_argument("original")
    args = parser.parse_args()

    if not os.path.exists(args.original):
        print("reference not found: %s" % args.original)
        print("Set HYPERX_REFERENCE in the CMake cache to the original executable.")
        return 0

    built = PEFile(args.built)
    original = PEFile(args.original)

    built_hash = hashlib.md5(built.data).hexdigest()
    original_hash = hashlib.md5(original.data).hexdigest()
    print("built    %s  %s" % (built_hash, args.built))
    print("original %s  %s\n" % (original_hash, args.original))

    if built_hash == original_hash:
        print("BYTE IDENTICAL")
        return 0

    field_table(built, original)
    section_table(built, original)
    first_difference(built, original)
    print("\nNot identical yet. This is expected until the sources are complete.")

    if built.timestamp != original.timestamp:
        print("\nFAIL: the COFF timestamp is %s, the original is %s."
              % (timestamp(built), timestamp(original)))
        print("The linker writes the clock. tools/stamp.py corrects it after every link,")
        print("so this means that step did not run. Run:")
        print("  python tools/stamp.py %s" % args.built)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
