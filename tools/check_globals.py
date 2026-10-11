"""Report GLOBAL markers that name one address twice, or that overlap each other.

reccmp resolves an operand through one name, so two declarations of one address make the
choice arbitrary. Overlap is the quieter version: declaring an int at 0x004ce96c when
0x004ce968 is already a double puts a second name inside the first one's storage, and
neither declaration looks wrong on its own.

Sizes come from the declared type. A type this does not know contributes no span and is
only checked for exact duplication.

    python tools/check_globals.py
"""
import collections
import glob
import os
import re
import sys

SIZES = {'char': 1, 'unsigned char': 1, 'signed char': 1, 'bool': 1,
         'short': 2, 'unsigned short': 2,
         'int': 4, 'unsigned int': 4, 'long': 4, 'unsigned long': 4, 'float': 4,
         'double': 8, '__int64': 8}

def span(decl):
    """Bytes the declaration occupies, or 0 when the type is not one we size."""
    text = re.sub(r'\b(?:extern|static|const|volatile)\b', ' ', decl.split('=')[0])
    head = text.replace(';', ' ').split('[')[0]
    bounds = re.findall(r'\[\s*(0x[0-9a-fA-F]+|\d+)\s*\]', text)
    if text.count('[') != len(bounds):
        return 0
    if '*' in head:
        size = 4
    else:
        size = SIZES.get(' '.join(re.sub(r'\w+\s*$', ' ', head).split()), 0)
    for bound in bounds:
        size *= int(bound, 0)
    return size

seen = collections.defaultdict(set)

for path in sorted(glob.glob(os.path.join('src', '**', '*.h'), recursive=True) + glob.glob(os.path.join('src', '**', '*.cpp'), recursive=True)):
    text = open(path, encoding='utf-8').read()
    for match in re.finditer(r'// GLOBAL: HYPERX (0x[0-9a-fA-F]{8})[ ]*[\r]?[\n]([^\r\n]+)', text):
        seen[int(match.group(1), 16)].add((os.path.basename(path), match.group(2).strip()))

faults = 0

for address in sorted(seen):
    if len(seen[address]) > 1:
        faults += 1
        print('0x%08x declared %d times' % (address, len(seen[address])))
        for name, decl in sorted(seen[address]):
            print('    %s: %s' % (name, decl))

ordered = sorted(seen)
paired = 0

for i, address in enumerate(ordered[:-1]):
    reach = max(span(decl) for _, decl in seen[address])

    if not reach or address + reach <= ordered[i + 1]:
        continue

    # The pool counts overlap on purpose: the client reads a dword at one address and a word
    # two bytes into it, and both readers match exactly.
    both = seen[address] | seen[ordered[i + 1]]

    if all(name == 'poolcounts.h' for name, decl in both):
        paired += 1
        continue

    faults += 1
    print('0x%08x spans %d bytes and reaches 0x%08x' % (address, reach, ordered[i + 1]))
    for name, decl in sorted(seen[address]) + sorted(seen[ordered[i + 1]]):
        print('    %s: %s' % (name, decl))

print('%d addresses, %d faults, %d pool count pairs allowed'
      % (len(seen), faults, paired))
sys.exit(1 if faults else 0)
