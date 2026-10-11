"""Rank the functions nobody has written yet by how much of what they call already exists.

It covers the functions that carry no annotation at all, and reads each disassembly from
the Ghidra bridge that tools/ghidra_api.py names.

Functions under 32 bytes are left out. Most are SEH unwind funclets the compiler emits from
the parent function, which cannot be written as source.

    python tools/ready_unwritten.py           # 128 bytes and up
    python tools/ready_unwritten.py 200
"""
import concurrent.futures
import csv
import glob
import re
import sys
import urllib.request

from ghidra_api import BASE  # noqa: E402

FPU = re.compile(r'^(F[A-Z]|FN)')


def sizes():
    out = {}
    for row in csv.DictReader(open('symbols/functions.csv', encoding='utf-8')):
        out[int(row['address'], 16)] = (int(row['size']), row['name'])
    return out


def annotated():
    """Every original address this tree already covers, from its own annotations."""
    functions = set()
    globals_ = set()
    for path in glob.glob('src/**/*.cpp', recursive=True) + glob.glob('src/**/*.h', recursive=True):
        text = open(path, encoding='utf-8', newline='').read()
        for hit in re.finditer(r'//\s*(FUNCTION|LIBRARY|STUB|SYNTHETIC):\s*HYPERX\s+0x([0-9a-fA-F]{8})',
                               text):
            functions.add(int(hit.group(2), 16))
        for hit in re.finditer(r'//\s*(GLOBAL|STRING|VTABLE):\s*HYPERX\s+0x([0-9a-fA-F]{8})',
                               text):
            globals_.add(int(hit.group(2), 16))
    # Float constants are named in symbols/float_constants.csv. Its comment lines are dropped
    # so DictReader finds the real header row.
    rows = (line for line in open('symbols/float_constants.csv', encoding='utf-8')
            if not line.startswith('#'))
    for row in csv.DictReader(rows):
        if row.get('address', '').startswith('0x'):
            globals_.add(int(row['address'], 16))

    return functions, globals_


def unwritten():
    """Every original function no marker of any kind covers."""
    marked = set()
    for path in glob.glob('src/**/*.cpp', recursive=True) + glob.glob('src/**/*.h', recursive=True):
        text = open(path, encoding='utf-8', newline='').read()
        for hit in re.finditer(r'//\s*(FUNCTION|LIBRARY|STUB|SYNTHETIC):\s*HYPERX\s+0x([0-9a-fA-F]{8})',
                               text):
            marked.add(int(hit.group(2), 16))
    # A retired row was measured and given up, so it is not work.
    for line in open('symbols/retired.csv', encoding='utf-8'):
        if line.startswith('0x'):
            marked.add(int(line.split(',', 1)[0], 16))
    out = set()
    for row in csv.DictReader(open('symbols/functions.csv', encoding='utf-8')):
        address = int(row['address'], 16)
        if address not in marked and int(row['size']) >= 32:
            out.add(address)
    return out


def survey(address):
    url = '%s/disassemble_function?address=0x%x' % (BASE, address)
    try:
        text = urllib.request.urlopen(url, timeout=30).read().decode('utf-8', 'replace')
    except Exception:
        return address, None

    calls = set()
    data = set()
    jumps = []
    imports = set()
    seen = set()
    back = 0
    fpu = 0
    seh = 0
    framed = 0
    table = 0
    bools = 0
    dup = 0
    saved = 0
    selector = 0
    frame = 0
    shapes = []
    for number, line in enumerate(text.strip().split('\n')):
        if ':' not in line:
            continue
        here = line.split(':', 1)[0].strip()
        body = line.split(':', 1)[1].strip()
        seen.add(int(here, 16))
        if FPU.match(body):
            fpu += 1
        if 'FS:[0x0]' in body:
            seh = 1
        # A frame pointer in the first two instructions is /Od, and a function written for
        # /Od cannot get close at /O2 however faithful the source.
        if number < 2 and body.startswith('MOV EBP,ESP'):
            framed = 1
        if number < 6 and re.match(r'PUSH E(BX|BP|SI|DI)$', body):
            saved += 1
        if number < 8 and frame == 0:
            hit = re.match(r'SUB\s+ESP,0x([0-9a-f]+)$', body)
            if hit:
                frame = int(hit.group(1), 16)
        hit = re.match(r'CALL\s+0x([0-9a-f]+)', body)
        if hit:
            calls.add(int(hit.group(1), 16))
        hit = re.match(r'JMP\s+0x([0-9a-f]+)', body)
        if hit:
            jumps.append(int(hit.group(1), 16))
        # An indirect call is a Win32 import thunk, not a global this tree names.
        hit = re.match(r'CALL\s+dword ptr \[0x00([0-9a-f]{6})\]', body)
        if hit:
            imports.add(int('00' + hit.group(1), 16))
        # A call through a scaled-index table keeps a raw address on both sides of the
        # diff, so the function cannot reach 100 however faithful the source.
        if re.match(r'CALL\s+dword ptr \[\w+\*[^\]]*\+', body):
            table = 1
        if re.match(r'SBB\s+(E\w\w),\1$', body):
            bools += 1
        shapes.append(re.sub(r'0x[0-9a-f]+', 'N', body))
        # `shr reg, 0x10` against 0xfedc is the plan selector test. It compiles to a
        # mask here whatever the source says, so the function cannot reach 100.
        if re.match(r'CMP\s+E\w\w,0xfedc$', body):
            selector = 1
        for found in re.finditer(r'\[0x00([0-9a-f]{6})\]', body):
            data.add(int('00' + found.group(1), 16))
        # An object reached as an immediate. The listing drops leading zeros. The trailing
        # boundary stops a float immediate such as 0x4d3ebc20 matching as an address.
        for found in re.finditer(r',\s*0x(4[bcd][0-9a-f]{4})(?![0-9a-f])', body):
            data.add(int(found.group(1), 16))
        hit = re.match(r'J\w+\s+0x([0-9a-f]+)', body)
        if hit and int(hit.group(1), 16) <= int(here, 16):
            back += 1
    # A tail call is a JMP landing outside every instruction the function owns.
    for target in jumps:
        if target not in seen:
            calls.add(target)

    data -= imports

    # Five copies of one six-instruction run is a switch whose arms differ only in a
    # constant, which this build merges into a single tail.
    windows = {}
    for start in range(len(shapes) - 6):
        key = ' | '.join(shapes[start:start + 6])
        windows[key] = windows.get(key, 0) + 1
    dup = 1 if windows and max(windows.values()) >= 5 else 0

    return address, (calls, data, back, fpu, seh, framed, table, bools, dup,
                     saved, selector, frame)


floor = int(sys.argv[1]) if len(sys.argv) > 1 else 128

size = sizes()
known_functions, known_globals = annotated()
todo = [a for a in sorted(unwritten()) if size.get(a, (0,))[0] >= floor]

rows = []
with concurrent.futures.ThreadPoolExecutor(max_workers=10) as pool:
    for address, result in pool.map(survey, todo):
        if not result:
            continue
        calls, data, back, fpu, seh, framed, table, bools, dup, saved, selector, frame = result
        missing_calls = sorted(c for c in calls if c not in known_functions)
        missing_data = sorted(d for d in data if d not in known_globals)
        total = len(calls) + len(data)
        have = total - len(missing_calls) - len(missing_data)
        ratio = 1.0 if total == 0 else have / float(total)
        rows.append((ratio, size[address][0], address, size[address][1],
                     missing_calls, missing_data, back, fpu, seh, framed,
                     table, bools, dup, saved, selector, frame))

ready = [r for r in rows if r[0] >= 0.999]

# Ranked by the shapes that predict a match. An unwind frame means the function is a destructor
# or holds an object with one, so writing it needs a class with base and member destructors.
def rank(row):
    _, total, _, _, _, _, back, fpu, seh, framed, table, bools, dup, saved, selector, frame = row
    # `saved` is shown but not scored: it has not predicted a stall.
    risk = table + dup + selector + (1 if bools else 0) + (1 if frame >= 16 else 0)
    # x87 is shown but not ranked on: it has not been the obstacle in practice.
    return (risk, seh, framed, 1 if back else 0, -total)

ready.sort(key=rank)

print('%d unwritten functions of %d bytes and up' % (len(rows), floor))
print()
print('everything they touch is already named: %d functions, %d bytes'
      % (len(ready), sum(r[1] for r in ready)))
print('%6s  %-10s %-6s %-4s %-5s %-4s %-5s %-4s %-4s %-5s %-4s %-5s  %s'
      % ('bytes', 'address', 'unwind', '/Od', 'loops', 'x87', 'table', 'bool', 'dup',
         'saved', 'sel', 'frame', 'name'))
for (_, total, address, name, _, _, back, fpu, seh, framed, table, bools, dup,
     saved, selector, frame) in ready:
    print('%6d  0x%08x %-6s %-4s %-5s %-4s %-5s %-4s %-4s %-5d %-4s %-5d  %s'
          % (total, address, 'yes' if seh else '.', 'yes' if framed else '.',
             back or '.', fpu or '.', 'yes' if table else '.', bools or '.',
             'yes' if dup else '.', saved, 'yes' if selector else '.', frame,
             name[:40]))

rest = [r for r in rows if r[0] < 0.999]
rest.sort(key=lambda r: (-r[0], -r[1]))
print()
print('the rest, closest first, with what is missing')
for (ratio, total, address, name, mc, md, back, fpu, seh, framed, _, _, _, _,
     _, _) in rest[:14]:
    gap = ' '.join('f%06x' % c for c in mc[:3]) + ' ' + ' '.join('d%06x' % d for d in md[:3])
    print('%3d%% %6d  0x%08x  %-34s %s' % (ratio * 100, total, address, name[:34], gap.strip()))
