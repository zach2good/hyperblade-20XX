# Reports markers reccmp cannot resolve, and markers whose kind disagrees with the address.
#
# It reads reccmp's compare database, built from the binaries on disk, so it covers every
# marker kind and cannot go stale. A marker is UNRESOLVED when its address never pairs with a
# symbol in our PDB: the name it declares is absent from our build. check_lost_markers.py
# compares against HEAD, so it cannot see a marker written and deleted between two commits.
#
# A marker is WRONG KIND when its kind disagrees with the section at its address, in the
# original or in our build. FUNCTION, STUB and LIBRARY name code; GLOBAL, VTABLE and STRING
# name data; an executable section holds code. The original side catches a marker on the wrong
# address. The recompiled side catches the right address under a name of the wrong sort, such
# as a function name on a data slot.
#
# A GUARD is a function-local static's guard byte (`?$S1@...`) in our build with no original
# address. It has no declaration to mark, so symbols/static_guards.csv names it.
#
# WRONG BYTES is a marker or symbols CSV row that pairs an original address with LIBCMT data our
# build holds different bytes for. Library data is the same object code on both sides, so any
# dword that differs, other than two image addresses, means the original address is wrong.
#
# FLOAT is an operand that reads a float constant on one side and a placeholder on the other,
# from build/report.json: a row symbols/float_constants.csv lacks, or one it should not have.
#
# Takes about five seconds, nearly all of it in reccmp's ingest. Exits 1 on any finding.
#
#     python tools/check_markers_resolve.py

import collections
import glob
import logging
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# Kinds that name code, then kinds that name data. reccmp drops a repeated STRING marker, but
# the address stays in the database, so only a literal our build never emits is reported.
CODE_KINDS = ('FUNCTION', 'STUB', 'LIBRARY')
DATA_KINDS = ('GLOBAL', 'VTABLE', 'STRING')
KINDS = CODE_KINDS + DATA_KINDS

# The address is read to its end, not to eight digits: poolcounts.h writes ten.
MARKER = re.compile(r'//\s*(%s):\s*HYPERX\s+0x([0-9a-fA-F]+)' % '|'.join(KINDS))

IMAGE_SCN_MEM_EXECUTE = 0x20000000


def read_markers():
    """Every marker in the tree as (kind, address) -> first file that writes it."""
    found = {}
    for path in sorted(glob.glob('src/**/*.cpp', recursive=True)) + sorted(glob.glob('src/**/*.h', recursive=True)):
        text = open(path, encoding='utf-8', errors='replace').read()
        for kind, address in MARKER.findall(text):
            found.setdefault((kind, int(address, 16)), path)
    return found


def section_ranges(path):
    """(start, end, name, executable) per section, in virtual addresses."""
    from peinfo import PEFile

    image = PEFile(str(path))
    ranges = []
    for section in image.sections:
        start = image.image_base + section['vaddr']
        ranges.append((start, start + section['vsize'], section['name'],
                       bool(section['flags'] & IMAGE_SCN_MEM_EXECUTE)))
    return ranges


def section_at(ranges, address):
    for start, end, name, executable in ranges:
        if start <= address < end:
            return name, executable
    return None, None


def load_compare():
    """reccmp's own view of the two images, or a message saying why there is none."""
    # reccmp's ingest log would bury the findings; check_marker_errors.py reads that log.
    logging.disable(logging.CRITICAL)
    from pathlib import Path

    from reccmp.compare import Compare
    from reccmp.project.detect import RecCmpProject

    project = RecCmpProject.from_directory(Path.cwd())
    if project is None:
        raise RuntimeError('no reccmp project in this directory')
    target = project.get('HYPERX')
    return target, Compare.from_target(target)


markers = read_markers()

try:
    target, compare = load_compare()
except Exception as error:
    print('cannot build the compare database (%s); run from the repository root with the '
          'original binary configured in reccmp-user.yml' % error)
    sys.exit(1)

entities = {}
for entity in compare._db.get_all():
    if entity.orig_addr is not None:
        entities[entity.orig_addr] = entity

original_sections = section_ranges(target.original_path)
recompiled_sections = section_ranges(target.recompiled_path)

unresolved = []
wrong_kind = []

for (kind, address) in sorted(markers, key=lambda pair: (pair[1], pair[0])):
    path = markers[(kind, address)]
    wants_code = kind in CODE_KINDS

    name, executable = section_at(original_sections, address)
    if name is None:
        wrong_kind.append(('0x%08x is outside every section of the original' % address,
                           kind, path))
    elif executable != wants_code:
        wrong_kind.append(('0x%08x is in the original\'s %s, which holds %s' %
                           (address, name, 'code' if executable else 'data'), kind, path))

    entity = entities.get(address)
    if entity is None or entity.recomp_addr is None:
        unresolved.append((address, kind, path))
        continue

    name, executable = section_at(recompiled_sections, entity.recomp_addr)
    if name is None:
        wrong_kind.append(('0x%08x resolved to 0x%08x, outside every section of our build' %
                           (address, entity.recomp_addr), kind, path))
    elif executable != wants_code:
        wrong_kind.append(('0x%08x resolved to 0x%08x in our %s, which holds %s' %
                           (address, entity.recomp_addr, name,
                            'code' if executable else 'data'), kind, path))

from reccmp.compare.db import ImageId

guards = []
for entity in compare._db.all(ImageId.RECOMP):
    name = entity.get('name') or ''
    if name.startswith('?$S') and entity.orig_addr is None:
        guards.append((entity.recomp_addr, name))

# `0003:0000e920       ??_R0?AVtype_info@@@8      004cf920     LIBCMT:typinfo.obj`
MAP_ROW = re.compile(r'^\s*[0-9a-f]{4}:[0-9a-f]{8}\s+\S+\s+([0-9a-f]{8})\s.*\s(\w+):\S+\.obj\s*$',
                     re.I)
library = set()
for line in open('build/out/HYPERX_sym.map', encoding='latin-1'):
    found = MAP_ROW.match(line)
    if found:
        library.add(int(found.group(1), 16))


def is_image_address(value):
    return 0x00400000 <= value < 0x00500000


wrong_bytes = []
read_bytes = 0
for entity in compare._db.all(ImageId.ORIG):
    if entity.recomp_addr not in library or entity.get('type') not in (None, 2):
        continue
    if section_at(original_sections, entity.orig_addr)[1] is not False:
        continue
    span = min(entity.get('size') or 4, 64) // 4 * 4
    if span == 0:
        continue
    read_bytes += 1
    theirs = compare.orig_bin.read(entity.orig_addr, span)
    ours = compare.recomp_bin.read(entity.recomp_addr, span)
    for offset in range(0, span, 4):
        a = int.from_bytes(theirs[offset:offset + 4], 'little')
        b = int.from_bytes(ours[offset:offset + 4], 'little')
        if a != b and not (is_image_address(a) and is_image_address(b)):
            wrong_bytes.append((entity.orig_addr, entity.recomp_addr, entity.name, offset))
            break

import operand_residue

floats = operand_residue.float_faults()

for address, kind, path in unresolved:
    print('UNRESOLVED 0x%08x  %-8s %s' % (address, kind, path))

for message, kind, path in wrong_kind:
    print('WRONG KIND %-8s %s  %s' % (kind, message, path))

per_kind = collections.Counter(kind for kind, _ in markers)
print('%d markers over %s' % (len(markers), '/'.join(
    '%d %s' % (per_kind[kind], kind) for kind in KINDS if per_kind[kind])))
for address, name in guards:
    print('GUARD      0x%08x  %s has no row in symbols/static_guards.csv' % (address, name))
for address, ours, name, offset in wrong_bytes:
    print('WRONG BYTES 0x%08x  %s differs from ours at 0x%08x from byte %d' %
          (address, name, ours, offset))
for name, address, theirs, ours in floats:
    print('FLOAT      %s  %s: %s against %s' % (address, name, theirs, ours))

print('%d library data compared' % read_bytes)
print('%d unresolved, %d wrong kind, %d unpaired guards, %d wrong bytes, %d float rows' % (
    len(unresolved), len(wrong_kind), len(guards), len(wrong_bytes), len(floats)))

sys.exit(1 if unresolved or wrong_kind or guards or wrong_bytes or floats else 0)
