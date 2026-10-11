"""Check every inventory size against the number of bytes reccmp actually compared.

`progress.py` credits a row's declared size when reccmp scores it at a hundred. reccmp never
reads the inventory: it compares our function's own length, cut short at the next annotated
original address. So the metric is wrong wherever the two disagree:

  the row declares more than reccmp compared   bytes credited that nothing verified
  the row declares less than reccmp compared   bytes verified that nothing credits

A disagreement is not always a fault. Where our function comes from LIBCMT.LIB, the PDB's
symbol can be shorter than the original's function while every byte still matches, so read the
listing before changing a row.

    python tools/check_row_sizes.py
    python tools/check_row_sizes.py --threshold 8
"""
import argparse
import contextlib
import csv
import io
import json
import logging
import os
import pathlib
import sys

from reccmp.compare import Compare
from reccmp.compare.db import ImageId
from reccmp.project.detect import RecCmpProject

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INVENTORY = os.path.join(ROOT, 'symbols', 'functions.csv')
REPORT = os.path.join(ROOT, 'build', 'report.json')
GAME_CODE_LIMIT = 0x004A0000


def inventory():
    sizes = {}
    with open(INVENTORY, encoding='utf-8') as handle:
        for row in csv.DictReader(handle):
            if str(row['size']).isdigit():
                sizes[int(row['address'], 16)] = (int(row['size']), row['name'])
    return sizes


def compared_sizes():
    """The byte count reccmp used for each exactly matching original function.

    This is `orig_size` in `reccmp.compare.functions.compare_function`, reproduced
    rather than re-derived, so the number is the one the score was computed over.
    """
    logging.disable(logging.CRITICAL)
    project = RecCmpProject.from_directory(pathlib.Path(ROOT))
    # from_target prints one line per duplicate STRING annotation; silence it.
    noise = io.StringIO()
    with contextlib.redirect_stdout(noise):
        compare = Compare.from_target(project.get('HYPERX'))
    ratios = {}
    with open(REPORT, encoding='utf-8') as handle:
        for row in json.load(handle)['data']:
            ratios[int(row['address'], 16)] = row['matching']
    out = {}
    for match in compare._db.get_functions():
        address = match.orig_addr
        if ratios.get(address, 0.0) < 1.0:
            continue
        size = match.size(ImageId.ORIG)
        if size is None:
            recomp = match.size(ImageId.RECOMP)
            if recomp is None:
                continue
            limit = match.max_size(ImageId.ORIG)
            if limit is None:
                size = recomp
            else:
                size = min(limit, recomp)
        out[address] = size
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--threshold', type=int, default=1,
                        help='ignore disagreements smaller than this')
    args = parser.parse_args()

    if not os.path.exists(REPORT):
        print('no build/report.json. Run:')
        print('  python -m reccmp.tools.asmcmp --target HYPERX --json build/report.json --silent')
        return 1

    sizes = inventory()
    compared = compared_sizes()

    over = []
    under = []
    agree = 0
    for address, want in sorted(compared.items()):
        if address not in sizes:
            continue
        size, name = sizes[address]
        if want == size:
            agree += 1
        elif want < size:
            over.append((size - want, address, name, size, want))
        else:
            under.append((want - size, address, name, size, want))

    print('%d rows score exactly: %d agree, %d declare more than was compared, %d less'
          % (len(compared), agree, len(over), len(under)))
    lost = sum(d for d, a, _, _, _ in over if a < GAME_CODE_LIMIT and d >= args.threshold)
    gained = sum(d for d, a, _, _, _ in under if a < GAME_CODE_LIMIT and d >= args.threshold)
    print('game bytes credited exact that reccmp never compared : %d' % lost)
    print('game bytes reccmp matched that no row credits        : %d' % gained)
    for title, group in (('declares more than was compared', over),
                         ('declares less than was compared', under)):
        rows = [r for r in group if r[0] >= args.threshold]
        if not rows:
            continue
        print()
        print('%s:' % title)
        for delta, address, name, size, want in sorted(rows, reverse=True):
            tag = 'game' if address < GAME_CODE_LIMIT else 'crt '
            print('  %s 0x%08x %-40s row %5d  compared %5d  %+d'
                  % (tag, address, name[:40], size, want, want - size))
    return 0


if __name__ == '__main__':
    sys.exit(main())
