"""Find `*(T*)&x->field` where T is a different width than the field it reaches.

Read only. A cast such as `*(int*)&pBody->flFacing` keeps an integer MOV over a float, which is
correct only while the cast and the field are the same width.

    python tools/check_cast_width.py

A cast wider than its field reads bytes of the next field. A narrower cast reads or writes part
of one. Neither is automatically wrong: most rows read the low byte or half of a wider field
because the client emitted a narrow flag test or store, and the few wider rows stay in bounds and
carry a note at the declaration. Every row has been read, so the count is the tripwire: the tool
exits 1 when it differs from RESTING.

The base may be a local, `this`, or a member or global that a single declaration gives a pointer
type. A name with two declared types is dropped.

Limit: only a plain `name->field` or bare `m_field` is seen. A cast through a longer expression,
such as `((RokObject*)g_pGameEngine->m_ppSlots[6])->m_nCarrierSlot`, is not counted.
"""

import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import check_named_offsets as named
from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# `*(int*)&pFoo->m_bar` and `*(int*)&pFoo->m_bar[2][1]`
SITE = re.compile(r'\*\(\s*((?:unsigned |signed |const )*[A-Za-z_]\w*)\s*\*\s*\)\s*&\s*'
                  r'(\w+)\s*->\s*(m_\w+|[a-z]\w*)((?:\[[^\]]+\])*)')
# `*(short*)&m_bar[4]` inside a member body, whose owner is the enclosing class.
BARE = re.compile(r'\*\(\s*((?:unsigned |signed |const )*[A-Za-z_]\w*)\s*\*\s*\)\s*&\s*'
                  r'(m_\w+)\b((?:\[[^\]]+\])*)(?!\s*(?:->|\.|\[))')


MEMBER_DECL = re.compile(r"^[ \t]*(?:const\s+|volatile\s+|mutable\s+|struct\s+|class\s+)*"
                         r"([A-Za-z_]\w*)\s*\*\s*(m_\w+)\s*;", re.M)
GLOBAL_DECL = re.compile(r"^[ \t]*(?:extern\s+)?(?:const\s+|volatile\s+)*"
                         r"(?:class\s+|struct\s+)?([A-Za-z_]\w*)\s*\*\s*(g_\w+)"
                         r"\s*(?:=[^;]*)?;", re.M)


def wideNames():
    """Every member and global a single declaration gives a pointer type, mapped to it."""
    seen = {}

    for path in (sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.h"), recursive=True))
                 + sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"),
                                    recursive=True))):
        text = open(path, encoding="utf-8", errors="replace").read()
        text = code(text)

        for pattern in (MEMBER_DECL, GLOBAL_DECL):
            for kind, name in pattern.findall(text):
                seen.setdefault(name, set()).add(kind)

    return dict((name, list(kinds)[0]) for name, kinds in seen.items() if len(kinds) == 1)


# The expected count. Change it only after reading each new or removed row.
RESTING = 0


def main():
    fields, bases, access, spans = named.layouts()
    wide = wideNames()

    # field name -> declared element width, gathered per class from the spans and widths table
    width = {}
    for path in glob.glob(os.path.join(ROOT, "src", "**", "*.h"), recursive=True):
        cls = None
        for line in open(path, encoding="utf-8", errors="replace"):
            m = named.CLASS.match(line)
            if m:
                cls = m.group(1)
                continue
            m = named.FIELD.match(line)
            if m and cls:
                span = named.extent(m.group(1), m.group(3))
                if span:
                    # A subscripted site reaches an element, so a pointer field also records
                    # the width of what it points at.
                    ty = m.group(1).strip()
                    elem = span[0]
                    if ty.endswith("*"):
                        inner = named.extent(ty[:-1].strip(), "")
                        elem = inner[0] if inner else None
                    width.setdefault((cls, m.group(2)), (span[0], elem))

    bad = 0
    for path in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True)):
        cls, declared_as, cast_as = None, {}, {}
        for n, line in enumerate(open(path, encoding="utf-8", errors="replace"), 1):
            if line.lstrip().startswith("//"):
                continue
            if re.match(r'^\w[^;]*\([^;]*$', line) or re.match(r'^\w[^;]*\(.*\)\s*$', line):
                m = named.MEMBER.match(line)
                cls = m.group(1) if m else None
                declared_as, cast_as = {}, {}
            for t, v in named.DECL.findall(line):
                declared_as.setdefault(v, t)
            for t, v in named.CAST.findall(line):
                cast_as.setdefault(v, t)

            sites = SITE.findall(line)
            if cls:
                sites += [(cast, "this", field, subs) for cast, field, subs in BARE.findall(line)]
            for cast, base, field, subs in sites:
                if base == "this":
                    owner = cls
                else:
                    owner = (declared_as.get(base) or cast_as.get(base)
                             or wide.get(base))
                if not owner:
                    continue
                # the field may live on a base of the pointer's own class
                walk, got = owner, None
                for _ in range(16):
                    if (walk, field) in width:
                        got = width[(walk, field)][1 if subs else 0]
                        break
                    walk = bases.get(walk)
                    if not walk:
                        break
                if got is None:
                    continue
                want = named.extent(cast.strip(), "")
                if want and want[0] != got:
                    print("%-26s %5d  *(%s*)&%s->%s  reads %d, %s::%s is %d"
                          % (os.path.basename(path), n, cast.strip(), base, field + subs,
                             want[0], walk, field, got))
                    bad += 1

    print("%d cast(s) whose width disagrees with the field they reach, %d expected"
          % (bad, RESTING))

    # A drift either way fails: fewer means RESTING is stale, more means a new row to read.
    return 1 if bad != RESTING else 0


if __name__ == "__main__":
    sys.exit(main())
