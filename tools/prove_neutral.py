"""Prove a change moved no bytes, by holding every row against a snapshot taken before it.

    python tools/prove_neutral.py --save      # before the change, on a fresh measure
    <make the change, rebuild, measure again>
    python tools/prove_neutral.py             # after: fails if any row moved at all

It compares every row's matching value, not the average and not the exact set, so a row that
moves by a hundredth of a point fails it. The headline accuracy can hide a row that fell.

The snapshot lives beside the report in `build/`, outside the repository, and carries the
report's modification time. A comparison is refused when a source is newer than the linked
image or the report is older than the image, since either means the report does not describe
the current source.

This answers "did the bytes move". It does not replace the bring-up run, which answers whether
the game still works.
"""

import json
import os
import sys

REPORT = os.path.join('build', 'report.json')
SNAPSHOT = os.path.join('build', 'neutral_snapshot.json')

# The image the report is made from. `--target HYPERX` does not build it, so a source edited
# after the last full build reads as stale here exactly as it does in check_regressions.py.
IMAGE = os.path.join('build', 'out', 'HYPERX_sym.exe')


def stale():
    """Sources newer than the linked image, which is what a failed build looks like.

    A failed build leaves the previous image in place, and everything downstream reads it as
    clean. So a comparison is refused while any source is newer than the image. A snapshot is
    allowed, because taking one before the change is the point.
    """
    if not os.path.exists(IMAGE):
        return ['no %s at all' % IMAGE]

    linked = os.path.getmtime(IMAGE)
    late = []

    for root, unused_dirs, names in os.walk('src'):
        for name in names:
            if not name.endswith(('.c', '.cpp', '.h')):
                continue

            path = os.path.join(root, name)

            if os.path.getmtime(path) > linked:
                late.append(path)

    return late


def rows():
    """Every row's matching value, by address, out of the current report."""
    if not os.path.exists(REPORT):
        raise SystemExit('no %s; run the measure first' % REPORT)

    found = {}

    for row in json.load(open(REPORT, encoding='utf-8'))['data']:
        found[row['address']] = (float(row['matching']), row.get('name') or '')

    return found


def save():
    current = rows()

    open(SNAPSHOT, 'w', encoding='utf-8', newline='').write(json.dumps({
        'stamp': os.path.getmtime(REPORT),
        'rows': dict((address, current[address][0]) for address in current),
    }))

    print('snapshot of %d row(s) saved beside the report' % len(current))
    return 0


def compare():
    if not os.path.exists(SNAPSHOT):
        print('no snapshot; run --save before the change and this after it')
        return 1

    late = stale()

    if late:
        print('REFUSED: %d source(s) are newer than %s, so the report describes an image the '
              'build did not produce. A failed build leaves the old one and this would answer '
              'clean.' % (len(late), IMAGE))

        for path in sorted(late)[:8]:
            print('    %s' % path)

        return 1

    # The report must be newer than the image, or it describes the previous build.
    if os.path.getmtime(REPORT) < os.path.getmtime(IMAGE):
        print('REFUSED: %s is older than %s, so it describes the PREVIOUS build and nothing in '
              'it can have moved. Measure again, then run this.' % (REPORT, IMAGE))
        return 1

    saved = json.load(open(SNAPSHOT, encoding='utf-8'))

    if saved['stamp'] == os.path.getmtime(REPORT):
        print('the report has not been rebuilt since the snapshot, so this would compare a '
              'measurement against itself. Build and measure, then run this again.')
        return 1

    current = rows()
    before = saved['rows']
    moved = []
    gone = []
    added = []

    for address in sorted(current):
        if address not in before:
            added.append(address)
            continue

        if abs(current[address][0] - before[address]) > 1e-9:
            moved.append((address, before[address], current[address][0], current[address][1]))

    for address in sorted(before):
        if address not in current:
            gone.append(address)

    if not moved and not gone and not added:
        print('%d row(s), every one of them unchanged: the change moved no bytes'
              % len(current))
        return 0

    moved.sort(key=lambda row: abs(row[2] - row[1]), reverse=True)

    print('NOT NEUTRAL: %d row(s) moved, %d vanished, %d new'
          % (len(moved), len(gone), len(added)))
    print()

    for address, was, now, name in moved[:40]:
        print('    %-10s %7.2f%% -> %7.2f%%  %s' % (address, was * 100, now * 100, name[:46]))

    if len(moved) > 40:
        print('    ... and %d more' % (len(moved) - 40))

    for address in gone[:10]:
        print('    %-10s vanished from the report' % address)

    for address in added[:10]:
        print('    %-10s is new in the report' % address)

    return 1


def main():
    if '--save' in sys.argv:
        return save()

    return compare()


if __name__ == "__main__":
    sys.exit(main())
