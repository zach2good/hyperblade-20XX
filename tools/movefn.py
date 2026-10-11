import os
import re
import sys

os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import pragmas

# Moves marked definitions from one source file to another, in address order.
#
# Usage: movefn.py <target.cpp> <source.cpp> Class::Name [Class::Name ...]
# Each definition takes the comment block above its marker, and lands among the target's own
# markers in ascending address order.
#
# Pragmas that open directly above the body and close directly below it move with it. The move
# is refused, and neither file is written, when the body's pragma state reaches past it, or when
# the state at the target's insertion point differs from the state outside the body's pragmas.


def rd(p):
    return open(p, 'rb').read()


def wr(p, b):
    kind = b'\r\n' if b.count(b'\r\n') * 2 >= b.count(b'\n') else b'\n'
    b = b.replace(b'\r\n', b'\n').replace(b'\r', b'').replace(b'\n', kind)
    open(p, 'wb').write(b)

def nl_of(b):
    if b'\r\n' in b:
        return b'\r\n'
    return b'\n'


def lift(b, qual):
    if '::' in qual:
        cls, fn = qual.split('::')
        head = re.escape(cls.encode()) + rb'::' + re.escape(fn.encode())
    else:
        head = rb'(?<![:\w])' + re.escape(qual.encode())
    pat = re.compile(rb'// FUNCTION: HYPERX (0x[0-9a-f]+)\r?\n[^\n]*?' + head + rb'\(')
    m = pat.search(b)
    assert m, qual
    start = m.start()
    # walk back over the comment block directly above the marker
    while True:
        prev = b.rfind(b'\n', 0, start - 1)
        line = b[prev + 1:start].rstrip(b'\r\n')
        if line.startswith(b'//'):
            start = prev + 1
        else:
            break
    # the body ends at the first line that is exactly "}"
    end = re.compile(rb'\r?\n\}\r?\n').search(b, m.end())
    assert end, qual
    stop = end.end()
    lines = b.decode('latin1').split('\n')
    try:
        top, bottom, outside = pragmas.wrap(lines, b.count(b'\n', 0, start), b.count(b'\n', 0, stop))
    except pragmas.Ambiguous as e:
        sys.exit('refused %s: %s' % (qual, e))
    start = offset(b, top)
    stop = offset(b, bottom)
    block = b[start:stop]
    rest = b[:start] + b[stop:]
    rest = re.sub(rb'(\r?\n){3,}', lambda mm: mm.group(1) * 2, rest)
    return int(m.group(1), 16), block, rest, outside


def offset(b, line):
    """The byte offset where line number `line` starts."""
    at = 0
    for unused in range(line):
        at = b.index(b'\n', at) + 1
    return at


target = sys.argv[1]
source = sys.argv[2]
names = sys.argv[3:]
src = rd(source)
tgt = rd(target)
tnl = nl_of(tgt)
for qual in names:
    addr, block, src, outside = lift(src, qual)
    block = block.replace(b'\r\n', b'\n').replace(b'\n', tnl)
    if not block.endswith(tnl):
        block += tnl
    pos = None
    for mm in re.finditer(rb'// FUNCTION: HYPERX (0x[0-9a-f]+)', tgt):
        if int(mm.group(1), 16) > addr:
            p = mm.start()
            while True:
                prev = tgt.rfind(b'\n', 0, p - 1)
                line = tgt[prev + 1:p].rstrip(b'\r\n')
                if line.startswith(b'//'):
                    p = prev + 1
                else:
                    break
            tlines = tgt.decode('latin1').split('\n')
            pos = offset(tgt, pragmas.insertion(tlines, tgt.count(b'\n', 0, p)))
            break
    # Address order can land inside a namespace block, where VC6 rejects the definition with
    # C2888. If the last file-level `namespace` before the insertion point is still open, the
    # body goes after the block instead.
    if pos is not None:
        opened = None
        for mm in re.finditer(rb'^namespace\s+\w+', tgt[:pos], re.M):
            opened = mm.start()
        if opened is not None:
            depth = tgt.count(b'{', opened, pos) - tgt.count(b'}', opened, pos)
            if depth > 0:
                at = pos
                while depth > 0 and at < len(tgt):
                    step = tgt.find(b'}', at)
                    if step < 0:
                        break
                    depth -= 1 - tgt.count(b'{', at, step)
                    at = step + 1
                pos = tgt.find(b'\n', at)
                pos = len(tgt) if pos < 0 else pos + 1
    tlines = tgt.decode('latin1').split('\n')
    at_line = len(tlines)
    if pos is not None:
        at_line = tgt.count(b'\n', 0, pos)
    there = pragmas.state_at(tlines, at_line)
    if there != outside:
        sys.exit('refused %s: the target compiles it under [%s], the source under [%s]'
                 % (qual, there, outside))
    if pos is None:
        tgt = tgt.rstrip(b'\r\n') + tnl + tnl + block
    else:
        tgt = tgt[:pos] + block + tnl + tgt[pos:]
    print('moved', qual, hex(addr))
wr(source, src)
wr(target, tgt)
