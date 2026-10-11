"""Say what this tree calls the globals and functions a range of the image touches.

Ghidra and this tree often name the same thing differently. This takes an address range,
pulls every absolute address its code mentions, and prints the marker that claims each one, so
a decode can use the tree's names.

    python tools/name_at.py 0x44c910 0x61
"""
import glob
import os
import re
import struct
import subprocess
import sys

import capstone

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ghidra_api import BASE
from reference import reference_path

EXE = reference_path()
MARKER = re.compile(r'//\s*(FUNCTION|STUB|GLOBAL|VTABLE|STRING):\s*HYPERX\s+0x([0-9a-fA-F]{8})')


def sections():
    data = open(EXE, 'rb').read()
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    count = struct.unpack_from('<H', data, pe + 6)[0]
    optsize = struct.unpack_from('<H', data, pe + 20)[0]
    base = struct.unpack_from('<I', data, pe + 24 + 28)[0]
    out = []
    off = pe + 24 + optsize
    for i in range(count):
        name, vsize, va, rawsize, raw = struct.unpack_from('<8sIIII', data, off + i * 40)
        out.append((base + va, vsize, raw, rawsize))
    return data, out


DATA, SECS = sections()


def at(va, n):
    for sva, vsize, raw, rawsize in SECS:
        if sva <= va < sva + vsize and va - sva < rawsize:
            return DATA[raw + (va - sva):raw + (va - sva) + n]
    return None


def markers():
    found = {}
    for path in sorted(glob.glob('src/**/*.h', recursive=True)) + sorted(glob.glob('src/**/*.cpp', recursive=True)):
        lines = open(path, encoding='utf-8', newline='').read().replace('\r\n', '\n')
        pending = None
        for line in lines.split('\n'):
            hit = MARKER.search(line)
            if hit:
                pending = (int(hit.group(2), 16), hit.group(1))
                continue
            if pending is not None:
                name = re.search(r'([A-Za-z_][A-Za-z0-9_:~]*)\s*[\(\[;=,)]', line)
                if name:
                    found[pending[0]] = (pending[1], name.group(1), path.replace('src/', ''))
                pending = None
    return found


start = int(sys.argv[1], 16)
length = int(sys.argv[2], 0)
known = markers()
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
seen = []

for ins in md.disasm(at(start, length), start):
    for text in re.findall(r'0x4[0-9a-f]{5}', ins.op_str) + (
            [hex(ins.operands and 0)] if False else []):
        value = int(text, 16)
        if 0x401000 <= value < 0x500000 and value not in seen:
            seen.append(value)

for value in sorted(seen):
    entry = known.get(value)
    if entry:
        print('0x%08x  %-8s %-40s %s' % (value, entry[0], entry[1], entry[2]))
    else:
        out = subprocess.run(['curl', '-s',
                              '%s/get_function_by_address?address=0x%x' % (BASE, value)],
                             capture_output=True, text=True).stdout
        name = re.search(r'Function: (\S+)', out)
        print('0x%08x  %-8s %s' % (value, 'ABSENT', name.group(1) if name else '(data)'))
