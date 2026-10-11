"""Walk every .WBM the way ModelComponent::ReadAll does, and name the ones that run short.

A model whose blob list does not walk makes `ModelComponent::Read` answer null, and
`ModelComponent::ReadAll` raises "Read error loading ModelComponents". The bring-up skips the
fatal's shutdown, so the run goes on with a half-built model and crashes somewhere unrelated.
This answers the question without a build.

The layout, as src/graphics/modeldatabase.cpp reads it:

    48   ModelFileHeader, type and a 32 byte name and three version words
     2   LOD count
    20   five LOD thresholds
     n   a header blob, its own first dword being its size
     1   material count
    16   material file name
         material count * 16, the component names
         material count * 22, the material entries
     2   texture count
         texture count * 16, the texture names
         per LOD: blob count and total bytes, then that many blobs, each opening with its size

`ModelComponent::Read` reads a size and then that many bytes less four, so a blob whose size
overruns the file is the read that answers null.

    python tools/check_model_files.py          # fail on a file that is newly short
    python tools/check_model_files.py --all    # every file and its blob counts
    python tools/check_model_files.py --list   # the known-short files and nothing else

Two files on this disc do not walk and are listed in KNOWN: `NEMPTM.WBM`, loaded when Nemesis
is selected, and `CHISOM.WBM`, named by `CHICO.HTM` but reached by no fixture. The check fails on
a short file that is not listed, and on a listed file that now walks.
"""

import argparse
import glob
import os
import struct
import sys

MODELS = os.path.join('build', 'out', 'Data', 'MODELS', 'MDLSHD')

# Files this disc ships short. Remove a row in the change that makes its file walk.
KNOWN = {
    'CHISOM.WBM': 'named by CHICO.HTM and reached by no fixture, so it has never bitten',
    'NEMPTM.WBM': 'named by NEM.HTM, so selecting Nemesis loads it and the match dies',
}

HEADER_BYTES = 48
THRESHOLD_BYTES = 20
MATERIAL_NAME_BYTES = 16
MATERIAL_ENTRY_BYTES = 22
TEXTURE_NAME_BYTES = 16


def walk(path):
    """Every complaint the walk has about one file, and whether it finished."""
    data = open(path, 'rb').read()
    said = []

    at = HEADER_BYTES

    if at + 2 + THRESHOLD_BYTES > len(data):
        return ['shorter than its own header'], False

    lods = struct.unpack_from('<h', data, at)[0]
    at = at + 2 + THRESHOLD_BYTES

    at = at + struct.unpack_from('<I', data, at)[0]

    if at + 1 + MATERIAL_NAME_BYTES > len(data):
        return ['header blob runs past the end'], False

    materials = data[at]
    at = at + 1 + MATERIAL_NAME_BYTES
    at = at + materials * MATERIAL_NAME_BYTES
    at = at + materials * MATERIAL_ENTRY_BYTES

    if at + 2 > len(data):
        return ['material block runs past the end'], False

    at = at + 2 + struct.unpack_from('<h', data, at)[0] * TEXTURE_NAME_BYTES

    for lod in range(lods):
        if at + 8 > len(data):
            return said + ['LOD %d has no blob header' % lod], False

        blobs = struct.unpack_from('<I', data, at)[0]
        at = at + 8

        for index in range(blobs):
            if at + 4 > len(data):
                return said + ['LOD %d blob %d of %d: no size word, %d bytes left'
                               % (lod, index, blobs, len(data) - at)], False

            size = struct.unpack_from('<I', data, at)[0]

            if size < 4 or at + size > len(data):
                return said + ['LOD %d blob %d of %d: size %d overruns, %d bytes left'
                               % (lod, index, blobs, size, len(data) - at)], False

            at = at + size

        said.append('LOD %d: %d blobs' % (lod, blobs))

    said.append('%d bytes past the last blob' % (len(data) - at))

    return said, True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--all', action='store_true', help='every file, not just the short ones')
    parser.add_argument('--list', action='store_true', help='the known-short files and nothing else')
    parser.add_argument('--models', default=MODELS, help='where the .WBM files are')
    args = parser.parse_args()

    paths = sorted(glob.glob(os.path.join(args.models, '*.WBM')))

    if not paths:
        raise SystemExit('no .WBM files under %s; build the measured tree first' % args.models)

    if args.list:
        for name in sorted(KNOWN):
            print('%-16s %s' % (name, KNOWN[name]))

        return 0

    short = 0
    faults = 0
    walked = []

    for path in paths:
        try:
            said, whole = walk(path)
        except Exception as fault:
            said, whole = ['could not be walked: %s' % fault], False

        name = os.path.basename(path)

        if whole:
            walked.append(name)

            if not args.all:
                continue
        else:
            short = short + 1

            if name not in KNOWN:
                faults = faults + 1
                print('NEW SHORT  %-16s %s' % (name, said[-1]))

                continue

            if not args.all:
                continue

        print('%-16s %s' % (name, said[-1]))

    for name in sorted(KNOWN):
        if name in walked:
            faults = faults + 1
            print('WALKS NOW  %-16s remove its row from KNOWN in this file' % name)

    print('%d file(s), %d short, %d of them known, %d fault(s)'
          % (len(paths), short, short - faults if short >= faults else short, faults))

    return 1 if faults else 0


if __name__ == '__main__':
    sys.exit(main())
