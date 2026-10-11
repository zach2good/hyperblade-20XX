"""Find one original address claimed by more than one annotation.

Checks STUB, FUNCTION, GLOBAL, VTABLE and LIBRARY markers across src/ .cpp and .h files.
Exits 1 on any duplicate.

    python tools/duplicate_markers.py

Each collision is silent otherwise:
  - A stub left beside its written function mangles differently, so both link and callers
    that include the old header keep calling the stub.
  - A GLOBAL on both a header's extern and a source's definition: reccmp drops one, and the
    address resolves to neither name in the diff.
  - A LIBRARY and a FUNCTION on one address contradict each other. reccmp drops one with a
    line that `--silent` hides.

STRING markers are left out: the same literal is annotated wherever it is used, and reccmp
drops the repeats on purpose.
"""
import glob
import re
import sys
from collections import defaultdict

# Every kind reccmp resolves to an address, less STRING.
KINDS = ('STUB', 'FUNCTION', 'GLOBAL', 'VTABLE', 'LIBRARY')

# The address is read to its end, not to eight digits: some markers write ten.
MARKER = re.compile(r'//\s*(%s):\s*HYPERX\s+0x([0-9a-fA-F]+)' % '|'.join(KINDS))

seen = defaultdict(list)
for path in sorted(glob.glob('src/**/*.cpp', recursive=True) + glob.glob('src/**/*.h', recursive=True)):
    text = open(path, encoding='utf-8', newline='').read().replace('\r\n', '\n')
    for number, line in enumerate(text.split('\n'), 1):
        hit = MARKER.search(line)
        if hit:
            seen[int(hit.group(2), 16)].append((hit.group(1), path, number))

bad = {a: v for a, v in seen.items() if len(v) > 1}

for address in sorted(bad):
    kinds = sorted(set(kind for kind, _, _ in bad[address]))
    print('0x%08x claimed %d times, as %s' % (address, len(bad[address]), '/'.join(kinds)))
    for kind, path, number in bad[address]:
        print('    %-8s %s:%d' % (kind, path, number))

print('%d of %d annotated addresses carry more than one marker, over kinds %s'
      % (len(bad), len(seen), '/'.join(sorted(KINDS))))

sys.exit(1 if bad else 0)
