"""Report an address given more than one FUNCTION, STUB or LIBRARY marker.

reccmp pairs one address with one implementation. A second marker for an address the tree
already implements resolves the call target to whichever symbol the linker kept, and the
caller then differs by a name rather than by anything visible.

    python tools/check_functions.py
"""
import collections
import glob
import os
import re
import sys

seen = collections.defaultdict(set)

for path in sorted(glob.glob(os.path.join('src', '**', '*.cpp'), recursive=True) + glob.glob(os.path.join('src', '**', '*.h'), recursive=True)):
    text = open(path, encoding='utf-8').read()
    for match in re.finditer(r'// (FUNCTION|STUB|LIBRARY): HYPERX (0x[0-9a-fA-F]{8})', text):
        seen[match.group(2).lower()].add((os.path.basename(path), match.group(1)))

duplicates = {a: d for a, d in seen.items() if len(d) > 1}

for address in sorted(duplicates):
    print(address)
    for name, kind in sorted(duplicates[address]):
        print('    %s: %s' % (name, kind))

print('%d addresses, %d implemented twice' % (len(seen), len(duplicates)))
sys.exit(1 if duplicates else 0)
