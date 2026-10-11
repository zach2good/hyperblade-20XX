"""Find array globals declared shorter than the storage the image gives them.

An array declared too short matches on every byte reccmp compares, and fails only at run time
when the program subscripts past the decoded part. tools/check_globals.py covers the other
direction, a declaration that runs into the next global.

The tool reads the image between a declared array's end and the next annotated address, and asks
whether those bytes continue the array: a leading run of elements inside the range the declared
elements span, or inside the image's address range for a pointer array. A value that falls in
range by chance breaks the run. An array whose declared elements are all one value is skipped,
because a zeroed buffer is trivially in range. The gap may hold an unannotated global, so the
output is ranked candidates to read, and the exit code is 0 unless --strict.

    python tools/check_short_arrays.py            # candidates, strongest first
    python tools/check_short_arrays.py --all      # every array global and its gap
    python tools/check_short_arrays.py --strict   # exit non-zero when any candidate stands

Known false candidate: `g_aDefaultKeyBindings`. The bytes after it are a second table of the
same rows at 0x004c4c84, which nothing in the program references.
"""
import argparse
import glob
import os
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

SIZES = {'char': 1, 'unsigned char': 1, 'signed char': 1, 'bool': 1,
         'short': 2, 'unsigned short': 2,
         'int': 4, 'unsigned int': 4, 'long': 4, 'unsigned long': 4, 'float': 4,
         'double': 8, '__int64': 8}

FORMATS = {1: 'b', 2: 'h', 4: 'i'}

# A gap wider than this is a different global that nobody has annotated, not a tail.
GAP_LIMIT = 0x400

DECL = re.compile(r'// GLOBAL: HYPERX (0x[0-9a-fA-F]{8})[ ]*\r?\n([^\r\n]+)')
# Every dimension: a declaration of [2][3] is six elements.
ARRAY = re.compile(r'^(?:extern\s+)?(?:const\s+)?([A-Za-z_][\w ]*?)\s*(\**)\s*'
                   r'(?:const\s+)?(\w+)\s*((?:\[\s*\d+\s*\])+)')
DIMENSION = re.compile(r'\[\s*(\d+)\s*\]')

# A pointer array's elements are in range when they fall inside the image.
IMAGE_LOW = 0x00400000
IMAGE_HIGH = 0x00500000


def original():
    """The original image's bytes and its base address."""
    try:
        from reccmp.project.detect import RecCmpProject
    except ImportError:
        raise SystemExit('reccmp is not installed; pip install reccmp')

    project = RecCmpProject.from_directory(ROOT)

    if not project:
        raise SystemExit('no reccmp project at %s' % ROOT)

    found = project.get('HYPERX')

    if not Path(found.original_path).is_file():
        raise SystemExit('%s is missing' % found.original_path)

    from reccmp.formats import detect_image

    image = detect_image(Path(found.original_path))

    return image


def declarations():
    """Every annotated array global, as address -> (file, type, name, count, element size)."""
    found = {}

    for path in sorted(glob.glob(os.path.join('src', '**', '*.h'), recursive=True)
                       + glob.glob(os.path.join('src', '**', '*.cpp'), recursive=True)):
        text = open(path, encoding='utf-8', errors='replace').read()

        for match in DECL.finditer(text):
            decl = match.group(2).strip()
            parsed = ARRAY.match(decl)

            if not parsed:
                continue

            pointer = parsed.group(2) != ''

            if pointer:
                element = 4
            else:
                element = SIZES.get(' '.join(parsed.group(1).split()))

            if not element:
                continue

            address = int(match.group(1), 16)

            count = 1

            for size in DIMENSION.findall(parsed.group(4)):
                count = count * int(size)

            found[address] = (os.path.basename(path), parsed.group(1).strip(),
                              parsed.group(3), count, element, pointer)

    return found


def annotated():
    """Every annotated address of any kind, which is where a gap has to stop."""
    marks = set()
    pattern = re.compile(r'// (?:GLOBAL|VTABLE|STRING|FUNCTION|LIBRARY|STUB): '
                         r'HYPERX (0x[0-9a-fA-F]{8})')

    for path in sorted(glob.glob(os.path.join('src', '**', '*.h'), recursive=True)
                       + glob.glob(os.path.join('src', '**', '*.cpp'), recursive=True)):
        text = open(path, encoding='utf-8', errors='replace').read()

        for match in pattern.finditer(text):
            marks.add(int(match.group(1), 16))

    return sorted(marks)


def read(image, address, length):
    """The image's bytes at a virtual address, or None where it has none."""
    try:
        return image.read(address, length)
    except Exception:
        return None


def score(image, address, count, element, values, pointer):
    """How much the bytes past the declared end look like more of the same array."""
    marks = annotated()
    end = address + count * element
    # At or past the end: a neighbour annotated exactly at the end is why the array stops.
    following = [m for m in marks if m >= end]
    stop = following[0] if following else end + GAP_LIMIT
    gap = min(stop - end, GAP_LIMIT)
    abuts = following[0] if following else 0

    if gap < element or element not in FORMATS:
        return 0, 0, [], abuts

    gap = gap - gap % element
    raw = read(image, end, gap)

    if raw is None:
        return 0, 0, [], abuts

    tail = list(struct.unpack('<%d%s' % (gap // element, FORMATS[element]), raw))

    if not values or not tail:
        return 0, 0, [], abuts

    # At least two distinct declared values, so zeroed buffers are skipped.
    if len(set(values)) < 2:
        return 0, 0, [], abuts

    if pointer:
        low, high = IMAGE_LOW, IMAGE_HIGH
    else:
        low, high = min(values), max(values)
    inside = [v for v in tail if low <= v <= high]

    # The run of leading elements that stay in the declared range is what matters: a table's
    # tail is contiguous and a pointer that happens to fall in range is not.
    run = 0

    for value in tail:
        if low <= value <= high:
            run += 1
        else:
            break

    points = run * 2

    if run == len(tail):
        points += 4

    if len(inside) == len(tail):
        points += 2

    return points, run, tail, abuts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--all', action='store_true', help='every array global, not just candidates')
    parser.add_argument('--strict', action='store_true', help='exit non-zero on any candidate')
    args = parser.parse_args()

    image = original()
    arrays = declarations()

    rows = []

    for address in sorted(arrays):
        path, kind, name, count, element, pointer = arrays[address]

        if element not in FORMATS:
            continue

        raw = read(image, address, count * element)

        if raw is None:
            continue

        form = 'I' if pointer else FORMATS[element]
        values = list(struct.unpack('<%d%s' % (count, form), raw))
        points, run, tail, abuts = score(image, address, count, element, values, pointer)

        rows.append((points, run, address, path, kind, name, count, element, tail, values,
                     abuts))

    rows.sort(key=lambda row: (-row[0], row[2]))

    shown = 0

    for points, run, address, path, kind, name, count, element, tail, values, abuts in rows:
        if not args.all and points < 6:
            continue

        shown += 1

        owner = arrays.get(abuts)

        if owner is not None:
            note = ', and the run stops exactly at %s, so it is that table not a tail' % owner[2]
        elif abuts:
            note = ', up to the annotation at 0x%08x' % abuts
        else:
            note = ''

        print('0x%08x  %-34s %-28s declared %d, %d more in range%s'
              % (address, name, path, count, run, note))
        print('              declared %s' % ' '.join(str(v) for v in values))
        print('              follows  %s' % ' '.join(str(v) for v in tail[:16]))

    print('%d array global(s) read, %d candidate(s)' % (len(rows), shown))

    return 1 if args.strict and shown else 0


if __name__ == '__main__':
    sys.exit(main())
