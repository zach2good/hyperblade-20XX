"""Name the offsets a body reaches through a typed global.

Rewrites `*(short*)((char*)g_pGameEngine + 0x50)` to `g_pGameEngine->m_nViewedSlot` when a
header declares `g_pGameEngine` a `GameEngine*`. It follows tools/self_fields.py's rules:

  - The cast type matches the field: the cast goes and the field is named.
  - It does not: the cast stays and the field is named through it,
    `*(char***)&g_pGameEngine->m_ppSlots`.
  - `volatile` always stays. It forces a reload where the client reloads.
  - A strided read, `(char*)g + nIndex * 4 + 0x54`, is an element of an array declared at
    0x54 whose stride is the element's width.

It then folds two identities that read the same bytes:

  - `(SaveObject*)*(void**)&g->m_pSaveObject` is `g->m_pSaveObject`.
  - `((char**)&g->m_ppColObjects[0])[i]` is `*(char**)&g->m_ppColObjects[i]`.

It refuses, and reports:

  - A global a header does not declare `Klass* g_name`.
  - An offset the class does not declare, or one that is not a number.
  - A field that is not public, since the reader is not a member (C2248).
  - An offset inside a placeholder such as `m_gap5234` or `undecoded04`.
  - An access shape no pattern covers.

    python tools/global_fields.py --report g_pGameEngine GameEngine
    python tools/global_fields.py --report --diff g_pGameEngine GameEngine
    python tools/global_fields.py g_pGameEngine GameEngine --skip src/planvm/interactors.cpp
    python tools/global_fields.py g_pGameEngine GameEngine --files src/planvm/plancond.cpp,src/ui/screens/championshipsetupuiscreen.cpp
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import self_fields

LINE = chr(10)

DEFINE = re.compile(r"^#define ([0-9A-Za-z_]+) (0x[0-9a-fA-F]+)$", re.M)

RECORD = re.compile(r"^(class|struct) ([0-9A-Za-z_]+)(?: *: *public ([0-9A-Za-z_]+))? *"
                    + LINE + r"\{(.*?)" + LINE + r"\};", re.S | re.M)

# Any field name, m_ prefixed or not: not every record a global points at uses the prefix.
MEMBER = re.compile(r"^\s+(?:[\w:*&<>]+[\s*]+)+?([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?;")

CAST = r"((?:const |volatile |unsigned |signed )*)([0-9A-Za-z_]+)\s*(\*+)"
OFFSET = r"([0-9A-Za-z_]+(?:\s*\+\s*[0-9A-Za-z_]+)*)"

# A strided index: a name, or a cast call, `(short)pContext->GetModelSlot()`, with one level
# of parentheses inside the arguments. It is copied into the brackets verbatim.
INDEX = (r"((?:\(\s*\w+\s*\)\s*)?[A-Za-z_]\w*(?:(?:->|\.)[A-Za-z_]\w*)*"
         + r"(?:\((?:[^()]|\([^()]*\))*\))?)")

PLACEHOLDER = re.compile(r"^(?:m_)?a?(?:gap|undecoded|reserve)", re.I)


def sourceNames(root):
    # Sources live in subdirectories, so this walks. Names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def declared(name):
    """The class a header says `name` points at, or None."""
    for entry in sorted(sourceNames(SRC)):
        if not entry.endswith(".h"):
            continue

        text = open(os.path.join(SRC, entry), encoding="utf-8", errors="replace").read()
        hit = re.search(r"^extern\s+(?:(?:const|volatile)\s+)*(?:struct\s+|class\s+)?"
                        r"([A-Za-z_]\w*)\s*\*\s*(?:(?:const|volatile)\b\s*)*%s\s*(?:=[^;]*)?;"
                        % re.escape(name), text, re.M)

        if hit:
            return hit.group(1)

    return None


def header(klass):
    """The header that defines `klass`, which a file naming its fields must include.

    A file that sees only the forward declaration in src/globals.h fails with C2027.
    """
    for entry in sorted(sourceNames(SRC)):
        if not entry.endswith(".h"):
            continue

        text = open(os.path.join(SRC, entry), encoding="utf-8", errors="replace").read()

        if re.search(r"^(?:class|struct) %s\s*(?::[^{]*)?\r?$\s*\{" % re.escape(klass), text,
                     re.M):
            return entry

    return None


def include(text, entry):
    """`text` with `#include "entry"` added after its last quoted include, if it lacks one."""
    if re.search(r'^#include "%s"' % re.escape(entry), text, re.M):
        return text

    last = None

    for hit in re.finditer(r'^#include "[^"]+"[^\n]*\n', text, re.M):
        last = hit

    if last is None:
        return text

    return text[:last.end()] + '#include "%s"\n' % entry + text[last.end():]


def reachable(klass):
    """Field names a non-member may touch: the public ones, on the class and its bases."""
    names = set()

    for entry in sorted(sourceNames(SRC)):
        if not entry.endswith(".h"):
            continue

        text = open(os.path.join(SRC, entry), encoding="utf-8", errors="replace").read()
        text = text.replace(chr(13) + LINE, LINE)

        for hit in RECORD.finditer(text):
            kind, found, base, body = hit.groups()

            if found != klass:
                continue

            access = "public" if kind == "struct" else "private"

            for line in body.split(LINE):
                label = re.match(r"^(public|protected|private)\s*:", line.strip())

                if label:
                    access = label.group(1)
                    continue

                if access != "public":
                    continue

                field = MEMBER.match(line)

                if field:
                    names.add(field.group(1))

            if base:
                names |= reachable(base)

    return names


def spell(deref, qualifiers, kind, stars, field, declaredKind):
    """One access under the shared rules: the cast goes on a match and stays otherwise."""
    claimed = qualifiers.replace("volatile ", "") + kind + "*" * (len(stars) - 1)

    if claimed == declaredKind and "volatile" not in qualifiers:
        return field if deref == "*" else "&" + field

    return "%s(%s%s%s)&%s" % (deref, qualifiers, kind, stars, field)


def foldIdentity(text, name, fields):
    """`(T)*(U*)&g->p` is `g->p` when p is a pointer declared T and U is a pointer too.

    Only pointer fields: `(int)*(short*)&g->m_nX` reads two bytes and widens, and is not the
    field.
    """
    byName = dict((path, kind) for path, kind in fields.values())

    def fold(hit):
        outer = hit.group(1).strip()
        inner = hit.group(2).strip()
        field = hit.group(3)
        kind = byName.get(re.sub(r"\[\d+\]$", "[0]", field), byName.get(field))

        if kind is None or not kind.endswith("*") or not inner.endswith("*"):
            return hit.group(0)

        if outer != kind:
            return hit.group(0)

        return "%s->%s" % (name, field)

    return re.sub(r"\(([^()]+\*)\)\*\(([^()]+\*)\)&%s->([A-Za-z_]\w*(?:\[\d+\])?)"
                  % re.escape(name), fold, text)


def foldIndexed(text, name):
    """`((T*)&g->a[0])[i]` is `*(T*)&g->a[i]`, whatever i is."""
    pattern = re.compile(r"\(\(([^()]+\*)\)&%s->([A-Za-z_]\w*)\[0\]\)\[" % re.escape(name))
    out = []
    at = 0

    while True:
        hit = pattern.search(text, at)

        if hit is None:
            break

        depth = 1
        i = hit.end()

        while i < len(text) and depth:
            if text[i] == "[":
                depth += 1
            elif text[i] == "]":
                depth -= 1

            i += 1

        if depth:
            break

        index = text[hit.end():i - 1]
        out.append(text[at:hit.start()])
        out.append("*(%s)&%s->%s[%s]" % (hit.group(1), name, hit.group(2), index))
        at = i

    out.append(text[at:])

    return "".join(out)


def foldParens(text, name):
    """`(g->m)->f()` is `g->m->f()` and `(g->m)[i]` is `g->m[i]`: the group is a postfix base.

    Only those two. A group that is an operand stays: parentheses round one operand of a float
    multiply make VC6 load that operand first, so removing them changes the bytes.
    """
    return re.sub(r"\((%s->[A-Za-z_]\w*(?:\[[0-9A-Za-z_]+\])?)\)(?=\s*(?:->|\[))"
                  % re.escape(name), r"\1", text)


def rewrite(name, klass, fields, blocks, files, report, diff, missed):
    """Every file in `files`. Answers {file: named count} for the ones that changed."""
    home = header(klass)

    if home is None:
        print("no header declares the body of %s; refusing" % klass)

        return {}

    base = r"\(\s*char\s*\*\s*\)\s*" + re.escape(name)
    access = re.compile(r"(\*?)\(\s*" + CAST + r"\s*\)\s*\(\s*" + base + r"\s*\+\s*" + OFFSET
                        + r"\s*\)")
    strided = re.compile(r"(\*?)\(\s*" + CAST + r"\s*\)\s*\(\s*" + base + r"\s*\+\s*"
                         + r"(?:" + INDEX + r"\s*\*\s*([0-9A-Za-z_]+)\s*\+\s*([0-9A-Za-z_]+)"
                         + r"|([0-9A-Za-z_]+)\s*\+\s*" + INDEX + r"\s*\*\s*([0-9A-Za-z_]+))"
                         + r"\s*\)")
    bare = re.compile(r"\*\s*\(\s*" + base + r"\s*\+\s*" + OFFSET + r"\s*\)")
    leftover = re.compile(r".{0,30}\(\s*char\s*\*\s*\)\s*" + re.escape(name) + r"\s*\+[^;]{0,40}")
    headerMacros = self_fields.headerMacros()
    counts = {}

    for path in files:
        entry = os.path.basename(path)
        raw = open(path, "rb").read()
        crlf = chr(13) + LINE in raw.decode("utf-8", "replace")
        text = raw.decode("utf-8").replace(chr(13) + LINE, LINE)

        if name not in text:
            continue

        macros = dict(headerMacros)
        macros.update(dict((key, int(value, 16)) for key, value in DEFINE.findall(text)))
        named = [0]

        def place(expr, hit):
            offset = self_fields.total(expr, macros)

            if offset is None:
                missed.append((entry, hit.group(0)[:60], "offset is not a number"))

                return None

            if offset not in fields:
                missed.append((entry, hit.group(0)[:60], "0x%x undeclared" % offset))

                return None

            if PLACEHOLDER.match(fields[offset][0].split(".")[-1]):
                missed.append((entry, hit.group(0)[:60],
                               "0x%x is inside %s" % (offset, fields[offset][0])))

                return None

            return offset

        def swap(hit):
            offset = place(hit.group(5), hit)

            if offset is None:
                return hit.group(0)

            field, kind = fields[offset]
            named[0] += 1

            return spell(hit.group(1), hit.group(2), hit.group(3), hit.group(4),
                         "%s->%s" % (name, field), kind)

        def swapStrided(hit):
            if hit.group(5) is not None:
                index = hit.group(5)
                step = self_fields.term(hit.group(6), macros)
                offset = self_fields.term(hit.group(7), macros)
            else:
                offset = self_fields.term(hit.group(8), macros)
                index = hit.group(9)
                step = self_fields.term(hit.group(10), macros)

            if offset is None or step is None or offset not in blocks:
                missed.append((entry, hit.group(0)[:60], "strided, no array declared there"))

                return hit.group(0)

            field, kind, width, count = blocks[offset]

            if PLACEHOLDER.match(field):
                missed.append((entry, hit.group(0)[:60], "0x%x is inside %s" % (offset, field)))

                return hit.group(0)

            if step != width:
                missed.append((entry, hit.group(0)[:60],
                               "stride %d, array element is %d" % (step, width)))

                return hit.group(0)

            named[0] += 1

            return spell(hit.group(1), hit.group(2), hit.group(3), hit.group(4),
                         "%s->%s[%s]" % (name, field, index), kind)

        def swapBare(hit):
            offset = place(hit.group(1), hit)

            if offset is None:
                return hit.group(0)

            field, kind = fields[offset]
            named[0] += 1

            return spell("*", "", "char", "*", "%s->%s" % (name, field), kind)

        fixed = strided.sub(swapStrided, text)
        fixed = access.sub(swap, fixed)
        fixed = bare.sub(swapBare, fixed)

        if named[0]:
            fixed = foldIdentity(fixed, name, fields)
            fixed = foldIndexed(fixed, name)
            fixed = foldParens(fixed, name)

        if name + "->" in fixed:
            fixed = include(fixed, home)

        for hit in leftover.finditer(fixed):
            shown = hit.group(0).strip()

            if not any(shown in row[1] or row[1] in shown for row in missed if row[0] == entry):
                missed.append((entry, shown[:60], "shape not covered"))

        if fixed == text:
            continue

        counts[entry] = named[0]

        if diff:
            for old, new in zip(text.split(LINE), fixed.split(LINE)):
                if old != new:
                    print("  %s - %s" % (entry, old.strip()))
                    print("  %s + %s" % (entry, new.strip()))

        if not report:
            out = fixed.replace(LINE, chr(13) + LINE) if crlf else fixed
            open(path, "wb").write(out.encode("utf-8"))

    return counts


def listed(flag, argv):
    """The comma-separated paths after `flag`, resolved against the tree root."""
    if flag not in argv:
        return []

    raw = argv[argv.index(flag) + 1]

    return [os.path.normpath(os.path.join(ROOT, item)) for item in raw.split(",") if item]


def main():
    argv = sys.argv[1:]
    report = "--report" in argv
    diff = "--diff" in argv
    skip = set(listed("--skip", argv))
    only = listed("--files", argv)
    positional = []
    i = 0

    while i < len(argv):
        if argv[i] in ("--skip", "--files"):
            i += 2
            continue

        if not argv[i].startswith("--"):
            positional.append(argv[i])

        i += 1

    if len(positional) != 2:
        print(__doc__)

        return 1

    name, klass = positional
    found = declared(name)

    if found != klass:
        print("%s is declared `%s*` in the headers, not `%s*`; refusing" % (name, found, klass))

        return 1

    known = self_fields.records()
    fields = self_fields.layout(klass, known)

    if not fields:
        print("%s declares no fields this can read" % klass)

        return 1

    public = reachable(klass)
    fields = dict((at, pair) for at, pair in fields.items()
                  if pair[0].split(".")[0].replace("[0]", "") in public)
    blocks = dict((at, row) for at, row in self_fields.arrays(klass, known).items()
                  if row[0] in public)

    if only:
        files = only
    else:
        files = [os.path.join(SRC, entry) for entry in sorted(sourceNames(SRC))
                 if entry.endswith(".cpp")]

    files = [path for path in files if os.path.normpath(path) not in skip]
    missed = []
    counts = rewrite(name, klass, fields, blocks, files, report, diff, missed)

    for entry in sorted(counts):
        print("%4d  %s" % (counts[entry], entry))

    print("%d access(es) named in %d file(s), %d this cannot place%s"
          % (sum(counts.values()), len(counts), len(missed), " (report only)" if report else ""))

    for row in missed:
        print("    %-26s %-60s %s" % row)

    return 0


if __name__ == "__main__":
    sys.exit(main())
