# Builds, and adds the missing include to each failing file until the build passes.
#
# For each name the compiler cannot see, it adds the header that declares it (or, for an
# undefined type, the one that defines it). It stops when a round changes nothing. Includes go after the last include at conditional depth
# zero, never inside a bring-up block.

import glob
import os
import re
import subprocess
import sys

os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if os.environ.get('HB_TMP'):
    os.environ['TMP'] = os.environ['HB_TMP']
    os.environ['TEMP'] = os.environ['HB_TMP']


# C2582, "'operator =' function is unavailable", is how VC6 reports assigning a struct the file
# has only forward declared. Like C2027, it wants the defining header.
CODES = '2065|2027|2061|2504|2653|2512|2079|2501|2514|2582'
FULL = ('2027', '2504', '2653', '2512', '2079', '2501', '2514', '2582')


def rd(p):
    return open(p, 'rb').read()


def wr(p, b):
    open(p, 'wb').write(b)


FULL_FIRST = [os.environ.get('HB_FULL') == '1']


def build():
    text = ''
    for _ in range(4):
        args = ['cmake', '--build', 'build'] + (['--clean-first'] if FULL_FIRST[0] else [])
        FULL_FIRST[0] = False
        out = subprocess.run(args, capture_output=True, text=True, errors='ignore')
        text = out.stdout + out.stderr
        if 'U1054' not in text:
            break
    return text


def find_decl(ident, full):
    name = re.escape(ident.encode())
    if full:
        pats = [rb'^(?:class|struct|union)\s+' + name + rb'\s*(?::[^\n;]*)?\r?$', rb'^namespace ' + name + rb'\b']
    else:
        pats = [rb'^namespace ' + name + rb'\b',
                rb'^\s*(?:extern|class|struct|union|enum|typedef|#define)\b[^\n]*\b' + name + rb'\b[^\n]*',
                rb'^[A-Za-z_][^\n=]*\b' + name + rb'\s*\([^;{]*\)\s*;']
    for p in sorted(glob.glob('src/**/*.h', recursive=True)):
        b = rd(p)
        for pat in pats:
            if re.search(pat, b, re.M):
                return p.replace(os.sep, '/')
    return None


def insert_after_includes(text, nl, add):
    lines = text.split(nl)
    depth = 0
    last = -1
    for i, line in enumerate(lines):
        s = line.strip()
        if s.startswith('#ifndef') and i + 1 < len(lines) and lines[i + 1].strip().startswith('#define ' + s.split()[-1]):
            continue
        if s.startswith('#if'):
            depth += 1
        elif s.startswith('#endif'):
            depth = max(0, depth - 1)
        elif s.startswith('#include') and depth == 0:
            last = i
    lines[last + 1:last + 1] = [add]
    return nl.join(lines)


for round_ in range(40):
    text = build()
    if 'Built target hyperx_sym' in text and 'warning C4150' not in text:
        print('built')
        sys.exit(0)
    errs = re.findall(r'^(.*?\.(?:cpp|h))\((\d+)\) : error C(' + CODES.replace('|2079', '') + r"): [^']*'(\w+)'", text, re.M)
    errs += [(f, n, '2079', name) for f, n, name in re.findall(r"^(.*?\.(?:cpp|h))\((\d+)\) : error C2079: .*?undefined (?:class|struct|union) '(\w+)'", text, re.M)]
    errs += [(f, n, '2504', name) for f, n, name in re.findall(r"^(.*?\.(?:cpp|h))\((\d+)\) : error C2440: .*?cannot convert from '(?:class|struct) (\w+) \*'", text, re.M)]
    errs += [(f, n, '2079', name) for f, n, name in re.findall(r"^(.*?\.(?:cpp|h))\((\d+)\) : warning C4150: deletion of pointer to incomplete type '(\w+)'", text, re.M)]
    if not errs:
        print(text[-3000:])
        sys.exit(1)
    changed = False
    seen = set()
    for path, line, code, ident in errs:
        path = os.path.relpath(path).replace(os.sep, '/')
        if (path, ident) in seen:
            continue
        seen.add((path, ident))
        where = find_decl(ident, code in FULL)
        if not where or where == path:
            print('no declaration for', ident, 'in', path)
            continue
        b = rd(path)
        nl = '\r\n' if b'\r\n' in b else '\n'
        inc = where[4:]
        mydir = os.path.dirname(path)[4:]
        if mydir and inc.startswith(mydir + '/'):
            inc = inc[len(mydir) + 1:]
        line_text = '#include "%s"' % inc
        if line_text.encode() in b:
            print('already includes', inc, 'for', ident, 'in', path)
            continue
        wr(path, insert_after_includes(b.decode('latin1'), nl, line_text).encode('latin1'))
        print('added', inc, 'to', path)
        changed = True
    if not changed:
        print(text[-2000:])
        sys.exit(1)
print('gave up')
sys.exit(1)
