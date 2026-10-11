"""Find call targets the client's side has no annotation for.

reccmp names a call on our side from the PDB and on the client's from this tree's own
annotations. Where the client's target has none it renders `call <OFFSET1>`, our side renders
a name, and the two never pair, so the enclosing function loses that instruction.

The fix is a `LIBRARY` or `FUNCTION` marker, not a rewrite.

This walks every function that does not match, takes its CALL targets out of the client's
listing, and reports the ones no annotation covers, most-called first.

    python tools/unresolved_calls.py            # everything below 100 per cent
    python tools/unresolved_calls.py 80         # only functions already above 80
"""
import concurrent.futures
import glob
import json
import re
import sys
import urllib.request

from ghidra_api import BASE  # noqa: E402


def annotated():
    out = set()
    for path in glob.glob('src/**/*.cpp', recursive=True) + glob.glob('src/**/*.h', recursive=True):
        text = open(path, encoding='utf-8', newline='').read()
        for hit in re.finditer(r'//\s*(FUNCTION|LIBRARY|STUB):\s*HYPERX\s+0x([0-9a-fA-F]{8})',
                               text):
            out.add(int(hit.group(2), 16))
    return out


def targets(address):
    try:
        text = urllib.request.urlopen(
            '%s/disassemble_function?address=0x%x' % (BASE, address),
            timeout=30).read().decode('utf-8', 'replace')
    except Exception:
        return address, []
    found = []
    for line in text.split('\n'):
        hit = re.search(r'CALL\s+0x([0-9a-f]+)\s*$', line.strip())
        if hit:
            found.append(int(hit.group(1), 16))
    return address, found


floor = float(sys.argv[1]) / 100 if len(sys.argv) > 1 else 0.0

known = annotated()
todo = []
for entry in json.load(open('build/report.json'))['data']:
    address = int(entry['address'], 16)
    if floor <= entry['matching'] <= 0.999:
        todo.append(address)

print('checking %d functions that do not match' % len(todo))

callers = {}
with concurrent.futures.ThreadPoolExecutor(max_workers=10) as pool:
    for address, found in pool.map(targets, todo):
        for one in found:
            if one not in known:
                callers.setdefault(one, set()).add(address)

rows = sorted(callers.items(), key=lambda kv: -len(kv[1]))
print('%d call targets have no annotation, reached from %d functions'
      % (len(rows), len(set(a for v in callers.values() for a in v))))
print()
for target, from_ in rows[:30]:
    print('0x%08x  %2d callers  %s'
          % (target, len(from_), ' '.join('0x%x' % a for a in sorted(from_)[:6])))
