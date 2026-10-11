"""Find `+ 0xNN` computed on a pointer whose own class already declares a field at 0xNN.

Read only. A site counts only when its base is a local, member or parameter whose pointer type is
written in the same file, a cast of one, a call whose return type a header declares, or `this`
inside a member. Offsets repeat across sibling classes, so an offset alone never names a field.

Known limits. Read each reported site before acting on it:
- A cast binds a name until the end of the function, and one local can hold different records in
  turn. A later site can take the class of an earlier cast.
- It names the field at the offset, not the extent the site touches. A memset over a region
  reports the region's first field.
- It sees hex literals and `#define`s of hex literals in the same file only. A clean run does not
  prove a file clean.
- It ignores the width of the cast, so a reported site can read four bytes where the field is two.
- It cannot tell whether a base's field keeps its meaning on a derived class.

    python tools/check_named_offsets.py
    python tools/check_named_offsets.py --all    every site, not just the first per field
"""

import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from settled import isSettled

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

FIELD = re.compile(r'^\s+(?:const\s+)?([\w:]+[\w:\s\*]*?)\s+(m_\w+|[a-z]\w*)((?:\[[^\]]*\])*)\s*;\s*//\s*(0x[0-9a-f]+)')

# A field covers a range. Types whose element size is unknown match their first byte only.
WIDTH = {"char": 1, "unsigned char": 1, "signed char": 1, "bool": 1,
         "short": 2, "unsigned short": 2, "int": 4, "unsigned int": 4, "long": 4,
         "unsigned long": 4, "float": 4, "DWORD": 4, "double": 8}


def extent(ty, dims):
    """(element bytes, dimensions) for a declared field, or None when the type is unknown."""
    ty = ty.strip()
    if ty.endswith("*"):
        return 4, ()
    size = WIDTH.get(ty)
    if size is None:
        return None
    # A dimension can be an expression, such as `[0x60 - 0x5c]`. One that does not evaluate still
    # registers the field, with no span.
    out = []
    for d in re.findall(r'\[([^\]]*)\]', dims):
        d = d.strip()
        if not re.fullmatch(r'[0-9a-fxA-FX+\-*/() ]+', d):
            return size, ()
        try:
            out.append(int(eval(d, {"__builtins__": {}}, {})))
        except Exception:
            return size, ()
    return size, tuple(out)
CLASS = re.compile(r'^(?:class|struct)\s+(\w+)(?:\s*:\s*public\s+(\w+))?')
# The return type is optional because a constructor or destructor has none.
MEMBER = re.compile(r'^(?:[\w:\*& ]+?\s+)?(\w+)::~?\w+\s*\(')
# `Foo* p;`, `Foo* const p = ...`, and parameters `Foo* p` inside a signature.
DECL = re.compile(r'\b([A-Z]\w+)\s*\*\s*(?:const\s+)?(\w+)\s*[;,=]')
# A cast is a type claim too, and for a char* alias it is the only one. It is a separate pattern
# so that a cast of one variable does not bind to the name that follows it.
CAST = re.compile(r'\(\s*([A-Z]\w+)\s*\*\s*\)\s*(\w+)\b')
# An alias of `this`, such as `char* const pThis = (char*)this;`, is the enclosing class.
SELF = re.compile(r'(\w+)\s*=\s*\(\s*(?:char|void)\s*\*\s*\)\s*this\s*;')



def returnTypes(fields):
    """Method name -> the class its declared return type names, for methods whose name is
    unambiguous across the tree.

    This lets a call result be a base: `(char*)pOwner->GetLocalTransformPtr() + 0x08`.
    A name two classes declare with different return types is dropped.
    """
    seen, clash = {}, set()

    for path in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.h"), recursive=True)):
        for line in open(path, encoding="utf-8", errors="replace"):
            m = re.match(r"^\s*(?:virtual\s+)?(?:const\s+)?(?:class\s+|struct\s+)?(\w+)\s*\*\s*(\w+)\s*\(\s*\)", line)

            if not m:
                continue

            kind, method = m.group(1), m.group(2)

            if kind not in fields:
                continue

            if method in seen and seen[method] != kind:
                clash.add(method)

            seen[method] = kind

    for method in clash:
        del seen[method]

    return seen

def scalarStructs():
    """Simple structs whose members are all scalars: name -> [(byte offset, member, width)].

    A field of struct type covers a range: a `Vec3` at 0x50 also answers 0x54 and 0x58.
    """
    out = {}
    for path in glob.glob(os.path.join(ROOT, "src", "**", "*.h"), recursive=True):
        name, at, members, ok = None, 0, [], True
        for line in open(path, encoding="utf-8", errors="replace"):
            m = re.match(r'^struct\s+(\w+)\s*$', line)
            if m:
                name, at, members, ok = m.group(1), 0, [], True
                continue
            if name is None:
                continue
            if line.startswith("};"):
                if ok and members:
                    out.setdefault(name, members)
                name = None
                continue
            m = re.match(r'^\s+([A-Za-z_][\w ]*?)\s+(\w+)\s*;\s*$', line)
            if m:
                w = WIDTH.get(m.group(1).strip())
                if w is None:
                    ok = False
                else:
                    members.append((at, m.group(2), w))
                    at += w
            elif "virtual" in line:
                # a vptr at offset 0 would shift every member after it
                ok = False
            elif "(" in line or re.match(r'\s*(public|protected|private)\s*:', line):
                # member functions and access labels do not disqualify the struct
                pass
            elif line.strip() and not line.lstrip().startswith(("//", "{")):
                ok = False
    return out


def norm(off):
    """One spelling for an offset. The dicts are keyed by string, so `0x04` in a header and
    `0x4` from a site would be different keys without it."""
    return "0x%x" % int(off, 16)


def layouts():
    fields, bases, access, spans = {}, {}, {}, {}
    structs = scalarStructs()
    for path in glob.glob(os.path.join(ROOT, "src", "**", "*.h"), recursive=True):
        cls, recent = None, []
        section = "private"
        for line in open(path, encoding="utf-8", errors="replace"):
            m = CLASS.match(line)
            if m:
                cls, recent = m.group(1), []
                section = "public" if line.startswith("struct") else "private"
                # A forward declaration must not erase a real base recorded from the
                # definition in another header.
                if m.group(2):
                    bases[cls] = m.group(2)
                else:
                    bases.setdefault(cls, None)
                fields.setdefault(cls, {})
                continue
            if line.lstrip().startswith("//"):
                recent.append(line)
                del recent[:-12]
                continue
            hit = re.match(r'\s*(public|protected|private)\s*:', line)
            if hit:
                section = hit.group(1)
                recent = []
                continue
            m = FIELD.match(line)
            if m and cls and not isSettled("".join(recent)):
                off, name = m.group(4), m.group(2)
                off = norm(off)
                if fields.setdefault(cls, {}).setdefault(off, name) == name:
                    access[(cls, name)] = section
                    span = extent(m.group(1), m.group(3))
                    if span and span[1]:
                        spans.setdefault(cls, []).append((int(off, 16), name, span))
                    elif m.group(1).strip() in structs and not m.group(3):
                        spans.setdefault(cls, []).append(
                            (int(off, 16), name, ("struct", structs[m.group(1).strip()])))
                    # An embedded class is a span too, including one with an array member
                    # that scalarStructs() refuses.
                    elif m.group(1).strip() in fields and not m.group(3):
                        spans.setdefault(cls, []).append(
                            (int(off, 16), name, ("class", m.group(1).strip())))
            recent = []
    return fields, bases, access, spans


def mostDerived(bases, kinds):
    """The one kind every other is an ancestor of, or None when two are unrelated.

    A base and its derived class name the same field, so the most derived one wins. Two
    siblings are ambiguous, because one offset is a different field on each.
    """
    for want in kinds:
        ok = True
        for other in kinds:
            walk, found = want, False
            for _ in range(16):
                if walk == other:
                    found = True
                    break
                walk = bases.get(walk)
                if not walk:
                    break
            if not found:
                ok = False
                break
        if ok:
            return want
    return None


def inside(spans, bases, fields, cls, off):
    """A field whose range covers `off`, spelled with the subscripts that reach it.

    Every dimension is spelled: 0x0c into `float m_a[4][4]` is m_a[0][2], not m_a[2].
    Embedded classes are walked up to four levels deep, so a name can be m_local.m_a[1][3].
    The reported class stays the outermost one, where the first component is declared.
    """
    n, walk, prefix, top = int(off, 16), cls, "", None
    for _ in range(4):
        found, base = None, walk
        for _ in range(16):
            for start, name, shape in spans.get(base, []):
                if shape[0] == "struct":
                    for moff, member, width in shape[1]:
                        if start + moff == n and moff:
                            return top or base, prefix + "%s.%s" % (name, member)
                    continue
                if shape[0] == "class":
                    # bounded by the next field; the last field is bounded by nothing
                    later = [int(o, 16) for o in fields.get(base, {})
                             if int(o, 16) > start]
                    stop = min(later) if later else (1 << 30)
                    if start < n < stop:
                        found = (shape[1], start, name, base)
                        break
                    continue
                size, dims = shape
                total = size
                for d in dims:
                    total *= d
                if start < n < start + total and (n - start) % size == 0:
                    k = (n - start) // size
                    subs = []
                    for d in reversed(dims):
                        subs.append(k % d)
                        k //= d
                    return top or base, prefix + name + "".join("[%d]" % i
                                                                for i in reversed(subs))
            if found:
                break
            base = bases.get(base)
            if not base:
                break
        if not found:
            return None, None
        if top is None:
            top = found[3]
        walk, n, prefix = found[0], n - found[1], prefix + found[2] + "."
        # The start of an array reports its first element, since no site that wants a scalar
        # can name the whole array.
        for start, name, shape in spans.get(walk, []):
            if start == n and shape[0] not in ("struct", "class"):
                return top, prefix + name + "[0]" * len(shape[1])
        who, field = declared(fields, bases, walk, norm("0x%x" % n))
        if field:
            return top, prefix + field
    return None, None


def declared(fields, bases, cls, off):
    seen = 0
    while cls and seen < 16:
        got = fields.get(cls, {}).get(off)
        if got:
            return cls, got
        cls = bases.get(cls)
        seen += 1
    return None, None


def main():
    showAll = "--all" in sys.argv[1:]
    fields, bases, access, spans = layouts()
    returns = returnTypes(fields)
    hits, seen, blocked, ambig = 0, set(), [0], [0]

    for path in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True)):
        text = open(path, encoding="utf-8", errors="replace").read()
        defs = dict((n, v) for n, v in
                    re.findall(r'^#define\s+([A-Z][A-Z0-9_]*)\s+(0x[0-9a-fA-F]+)\s*$',
                               text, re.M))
        cls, declared_as, cast_as = None, {}, {}
        for n, line in enumerate(open(path, encoding="utf-8", errors="replace"), 1):
            # a comment is not a site
            if line.lstrip().startswith("//"):
                continue
            # Any definition at column zero ends the one before it, so a free function after a
            # member does not inherit the member's bindings.
            if re.match(r'^\w[^;]*\([^;]*$', line) or re.match(r'^\w[^;]*\(.*\)\s*$', line):
                m = MEMBER.match(line)
                cls = m.group(1) if m else None
                declared_as, cast_as = {}, {}
            for t, v in DECL.findall(line):
                if t in fields:
                    declared_as.setdefault(v, t)
            # Record every class a name is cast to. Casts to two siblings in one function make
            # the site ambiguous instead of taking the first.
            for t, v in CAST.findall(line):
                if t in fields:
                    cast_as.setdefault(v, set()).add(t)
            for v in SELF.findall(line):
                if cls:
                    cast_as.setdefault(v, set()).add(cls)
            # `(char*)p + 0x20` counts bytes whatever p is. A bare `p + 0x20` counts bytes only
            # when p is a char*, so a bare sum counts only when the name has no declared pointer
            # type here. The bare pattern also matches inside the cast one, so the cast sites are
            # excluded from it.
            byte = re.findall(r'\(\s*char\s*\*\s*\)\s*(\w+)\s*\+\s*(0x[0-9a-f]+|[A-Z][A-Z0-9_]*)((?:\s*[-+]\s*(?:0x[0-9a-f]+|\d+|[A-Z][A-Z0-9_]*))*)', line)
            # a call result whose return type a header declares
            calls = re.findall(r"\(\s*char\s*\*\s*\)\s*[\w>. ()\-]*?\->(\w+)\(\)\s*\+\s*(0x[0-9a-f]+|[A-Z][A-Z0-9_]*)", line)
            already = set((b, o) for b, o, _ in byte)
            bare = [(b, o, t) for b, o, t in
                    re.findall(r'\b(\w+)\s*\+\s*(0x[0-9a-f]+|[A-Z][A-Z0-9_]*)((?:\s*[-+]\s*(?:0x[0-9a-f]+|\d+|[A-Z][A-Z0-9_]*))*)', line)
                    if (b, o) not in already and b not in declared_as]
            # The method name stands in for the base; the resolver below takes its class
            # out of the return map rather than the declared or cast ones.
            fromCall = [("()" + m, o, "") for m, o in calls if m in returns]
            for base, off, tail in byte + bare + fromCall:
                if not off.startswith("0x"):
                    off = defs.get(off)
                    if off is None:
                        continue
                # Trailing terms, signed and possibly macros, are part of the address. A term
                # that does not resolve drops the site instead of counting as zero.
                total = int(off, 16)
                broken = False
                for sign, term in re.findall(r'([-+])\s*(0x[0-9a-f]+|\d+|[A-Z][A-Z0-9_]*)',
                                             tail):
                    if term.startswith("0x"):
                        val = int(term, 16)
                    elif term[0].isdigit():
                        val = int(term)
                    else:
                        named = defs.get(term)
                        if named is None:
                            broken = True
                            break
                        val = int(named, 16)
                    total += val if sign == "+" else -val
                if broken:
                    continue
                off = norm("0x%x" % total)
                if base.startswith("()"):
                    owner = returns.get(base[2:])
                else:
                    owner = declared_as.get(base)
                if not owner:
                    kinds = cast_as.get(base)
                    if kinds:
                        owner = mostDerived(bases, kinds)
                        if not owner:
                            print("AMBIG   %-24s %5d  %s + %s  ->  cast to %s here, and"
                                  " they are not one chain"
                                  % (os.path.basename(path), n, base, off,
                                     " and ".join(sorted(kinds))))
                            ambig[0] += 1
                            continue
                if not owner and base == "this":
                    owner = cls
                if not owner:
                    continue
                who, field = declared(fields, bases, owner, off)
                if not field:
                    who, field = inside(spans, bases, fields, owner, off)
                if not field:
                    continue
                key = (who, field)
                if key in seen and not showAll:
                    continue
                seen.add(key)
                # the bare field name: `m_a[1][2]` and `velDir.y` both look it up as the
                # member that was declared, not as the path that reached into it
                how = access.get((who, re.split(r"[\[.]", field)[0]), "private")
                # A private or protected field read from outside its class is still a true
                # finding, but taking it needs the field opened or an accessor.
                reach = "ok     "
                if how != "public":
                    reaches, walk = False, cls
                    for _ in range(16):
                        if walk == who:
                            reaches = True
                            break
                        walk = bases.get(walk)
                    if not (reaches and (how == "protected" or cls == who)):
                        reach = "BLOCKED"
                        blocked[0] += 1
                print("%s %-24s %5d  %s + %s  ->  %s::%s (%s)"
                      % (reach, os.path.basename(path), n, base, off, who, field, how))
                hits += 1

    print("%d site(s) computing an offset their own class already names, %d unreachable "
          "from where they are, %d ambiguous" % (hits, blocked[0], ambig[0]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
