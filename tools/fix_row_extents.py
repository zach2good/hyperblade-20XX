"""Give every matching row the extent reccmp actually compared, and drop what that swallows.

tools/progress.py credits a row's declared size when reccmp scores it at 100, but reccmp compares
the function's own length, cut at the next annotated address. Where the two disagree, bytes are
credited unverified or verified uncredited.

The usual cause is a `__try`: Ghidra ends the function at the first funclet and files each
funclet as a `Catch@` row of its own. This extends the parent's row over its funclets and
deletes their rows.

    python tools/fix_row_extents.py            print the plan
    python tools/fix_row_extents.py --apply    write symbols/functions.csv
"""
import argparse
import csv
import io
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from check_row_sizes import GAME_CODE_LIMIT, INVENTORY, compared_sizes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply', action='store_true')
    args = parser.parse_args()

    with open(INVENTORY, encoding='utf-8', newline='') as handle:
        header = handle.readline()
        body = handle.read()
    rows = []
    for row in csv.DictReader(io.StringIO(header + body)):
        rows.append(row)
    sizes = {}
    for row in rows:
        if str(row['size']).isdigit():
            sizes[int(row['address'], 16)] = int(row['size'])

    compared = compared_sizes()
    grown = []
    for address, size in sorted(sizes.items()):
        want = compared.get(address)
        if want is None or want <= size:
            continue
        grown.append((address, size, want))

    # Whatever an extended row now covers must not be counted a second time.
    starts = sorted(sizes)
    swallowed = []
    for address, size, want in grown:
        for other in starts:
            if other <= address or other >= address + want:
                continue
            if other + sizes[other] > address + want:
                swallowed.append((address, other, sizes[other], 'PARTIAL'))
            else:
                swallowed.append((address, other, sizes[other], 'inside'))

    partial = [s for s in swallowed if s[3] == 'PARTIAL']
    drop = set(other for _, other, _, kind in swallowed if kind == 'inside')

    added = sum(want - size for address, size, want in grown
                if address < GAME_CODE_LIMIT)
    removed = sum(sizes[a] for a in drop if a < GAME_CODE_LIMIT)
    names = dict((int(row['address'], 16), row['name']) for row in rows
                 if str(row['size']).isdigit())

    print('%d rows stop short of what reccmp compared' % len(grown))
    print('%d rows fall wholly inside an extended row and would be counted twice'
          % len(drop))
    print('%d rows straddle an extended row, which is a fault to read by hand'
          % len(partial))
    print()
    print('game bytes added to matched and to the denominator : %d' % added)
    print('game bytes the deletions take back off both        : %d' % removed)
    print('net change to the denominator                      : %+d' % (added - removed))
    print('net change to the matched total                    : %+d' % added)
    print()
    for address, size, want in grown:
        tag = 'game' if address < GAME_CODE_LIMIT else 'crt '
        print('%s 0x%08x %-40s %5d -> %5d' % (tag, address, names[address][:40],
                                              size, want))
        for parent, other, othersize, kind in swallowed:
            if parent == address:
                print('        %s 0x%08x %-30s %5d'
                      % (kind, other, names[other][:30], othersize))
    if partial:
        print()
        print('straddling rows, not touched:')
        for parent, other, othersize, _ in partial:
            print('  0x%08x is inside 0x%08x but ends past it' % (other, parent))

    if not args.apply:
        print()
        print('nothing written, pass --apply')
        return 0

    keep = []
    for row in rows:
        address = int(row['address'], 16)
        if address in drop:
            continue
        want = compared.get(address)
        if want is not None and str(row['size']).isdigit() and want > int(row['size']):
            row['size'] = str(want)
        keep.append(row)

    fields = list(rows[0].keys())
    out = io.StringIO()
    writer = csv.DictWriter(out, fieldnames=fields, lineterminator='\n')
    writer.writeheader()
    for row in keep:
        writer.writerow(row)
    with open(INVENTORY, 'w', encoding='utf-8', newline='') as handle:
        handle.write(out.getvalue())
    print()
    print('wrote %s: %d rows, %d dropped' % (INVENTORY, len(keep), len(rows) - len(keep)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
