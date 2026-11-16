"""Compare a built image against the original and report the first difference.

Reports header fields, then the section table, then the first differing byte in each
section that both images have, then each DLL's imports: whether the names agree, how many IAT
slots hold the same name at the same address, and the hints that differ.

    python tools/pecompare.py build/out/HYPERX.exe .../HYPERX.EXE
    python tools/pecompare.py --selftest

Exits non-zero only when the COFF timestamp differs. Other fields may differ while the sources
are incomplete. The timestamp may not: `tools/stamp.py` patches it after every link, so a
mismatch means that step did not run.
"""

import argparse
import datetime
import hashlib
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from peinfo import PEFile


def timestamp(image):
    stamp = image.timestamp
    if stamp == 0xFFFFFFFF:
        return "%#010x (reproducible)" % stamp
    moment = datetime.datetime.fromtimestamp(stamp, datetime.UTC)
    return "%#010x (%s)" % (stamp, moment.strftime("%Y-%m-%d %H:%M"))


def optional_word(image, offset, size):
    """A field of the optional header, `size` bytes at `offset`."""
    return struct.unpack_from("<H" if size == 2 else "<I", image.data, image.optional + offset)[0]


def image_version(image):
    return "%d.%d" % (optional_word(image, 44, 2), optional_word(image, 46, 2))


def stack(image):
    return "%#x / %#x" % (optional_word(image, 72, 4), optional_word(image, 76, 4))


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
        ("image version", image_version(built), image_version(original)),
        ("stack", stack(built), stack(original)),
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


def imports(path):
    """Answers {DLL: {IAT slot address: (name, hint)}} for an image."""
    import pefile

    image = pefile.PE(path, fast_load=True)
    image.parse_data_directories(
        directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_IMPORT"]])
    found = {}
    for entry in getattr(image, "DIRECTORY_ENTRY_IMPORT", []):
        slots = found.setdefault(entry.dll.decode().upper(), {})
        for symbol in entry.imports:
            name = symbol.name.decode() if symbol.name else "#%d" % symbol.ordinal
            slots[symbol.address] = (name, symbol.hint)

    return found


def compare_imports(built, original):
    """Answers (DLL, names agree, slots agreeing, the original's slot count, [(name, our hint,
    the original's hint)]) per DLL either image imports."""
    rows = []
    for dll in sorted(set(original) | set(built)):
        theirs = original.get(dll, {})
        ours = built.get(dll, {})
        same = sum(1 for address, (name, _) in theirs.items()
                   if ours.get(address, (None,))[0] == name)
        our_hints = {name: hint for name, hint in ours.values()}
        hints = sorted((name, our_hints[name], hint) for name, hint in theirs.values()
                       if name in our_hints and our_hints[name] != hint)
        names = {name for name, _ in ours.values()} == {name for name, _ in theirs.values()}
        rows.append((dll, names, same, len(theirs), hints))

    return rows


def import_table(built, original):
    print()
    print("%-14s %-6s %s" % ("imports", "names", "slots at the original's address"))
    for dll, names, same, total, hints in compare_imports(imports(built), imports(original)):
        mark = " " if names and same == total else "*"
        print("%s%-13s %-6s %d of %d" % (mark, dll, "same" if names else "differ", same, total))
        for name, ours, theirs in hints:
            print("    hint %s: %d, the original's %d" % (name, ours, theirs))


def selftest():
    original = {"KERNEL32.DLL": {0x100: ("Sleep", 5), 0x104: ("ExitProcess", 2)}}
    built = {"KERNEL32.DLL": {0x100: ("ExitProcess", 2), 0x104: ("Sleep", 7)}}
    found = compare_imports(built, original)
    assert found == [("KERNEL32.DLL", True, 0, 2, [("Sleep", 7, 5)])], found
    built["KERNEL32.DLL"] = {0x100: ("Sleep", 5), 0x104: ("ExitProcess", 2)}
    assert compare_imports(built, original) == [("KERNEL32.DLL", True, 2, 2, [])]
    # A DLL only one side imports reads as differing names.
    built["WINMM.DLL"] = {0x200: ("timeGetTime", 1)}
    assert compare_imports(built, original)[1] == ("WINMM.DLL", False, 0, 0, [])
    print("selftest passed")
    return 0


def main():
    if sys.argv[1:] == ["--selftest"]:
        return selftest()

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
    import_table(args.built, args.original)
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
