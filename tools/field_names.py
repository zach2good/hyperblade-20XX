"""Rewrite byte-offset reads of a class's own fields as the field names.

An access such as `*(float*)(pPlayer + PLAYER_ACCUM)`, where `pPlayer` is `(char*)this`, becomes
the field name when the class (or a base) declares a field at that offset with that exact type.
Everything skipped is reported.

    python tools/field_names.py Widget EngineObject      # rewrite these classes
    python tools/field_names.py --report Widget          # say what it would do
    python tools/field_names.py --count                  # count accesses left in the tree

    --launder   also rewrite a pointer field read through another pointer type, or a scalar read
                at its own width under another type, keeping the cast
    --address   also rewrite a typed address into the object as `&m_field`
    --index     also rewrite an access inside an array field as a subscript

An access through a local is only rewritten when that local is the object: the body assigns
`(char*)this` to it and never moves or rebinds it.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

CLASS = re.compile(r"^(?:class|struct) ([0-9A-Za-z_]+)(?:\s*:\s*public ([0-9A-Za-z_]+))?\s*"
                   + chr(10) + r"\s*\{(.*?)" + chr(10) + r"\};", re.S | re.M)

# The type must not reach back over an access specifier such as `public:`.
FIELD = re.compile(r"^[ 	]+(?!virtual|typedef)"
                   + r"((?:(?!p(?:ublic|rotected|rivate)\s*:)[\w:*&<>])[\w:*&<>\s]*?)"
                   + r"[\s*]([0-9A-Za-z_]+)(\[[^\]]*\])?;[ 	]*//\s*(0x[0-9a-fA-F]+)",
                   re.M)


DEFINITION = re.compile(r"^(?:[\w:*&<>\s]+?)\b([0-9A-Za-z_]+)::([~0-9A-Za-z_]+)\s*\([^;{]*\)"
                        + r"\s*(?:const\s*)?" + chr(10) + r"?\{", re.M)

HOLDER = re.compile(r"([0-9A-Za-z_]+)\s*=\s*\(\s*(?:const\s+)?char\s*\*\s*\)\s*this\s*;")
MOVED = re.compile(r"([0-9A-Za-z_]+)\s*(?:\+=|-=|\+\+|--)")


def sourceNames(root):
    # Sources live in subdirectories. Names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def sources(suffixes):
    for name in sorted(sourceNames(SRC)):
        if name.endswith(suffixes):
            text = open(os.path.join(SRC, name), encoding="utf-8", errors="replace").read()

            yield name, text.replace(chr(13) + chr(10), chr(10))


def fields():
    """{class: {offset: (declared type, name)}}, including every field a base declares.

    A derived declaration wins over the base's at the same offset.
    """
    found = {}
    base = {}

    for unused, text in sources((".h", ".cpp")):
        for match in CLASS.finditer(text):
            if match.group(1) in found:
                continue

            base[match.group(1)] = match.group(2)
            here = {}

            for kind, name, array, offset in FIELD.findall(match.group(3)):
                # An array field would match its element type; arrays() handles those.
                if array:
                    continue

                stars = name.count("*")
                kind = (kind + "*" * stars).replace(" ", "")
                here[int(offset, 16)] = (kind, name.lstrip("*"))

            found[match.group(1)] = here

    for klass in found:
        seen = set()
        parent = base.get(klass)

        while parent in found and parent not in seen:
            seen.add(parent)

            for offset, row in found[parent].items():
                found[klass].setdefault(offset, row)

            parent = base.get(parent)

    return dict((k, v) for k, v in found.items() if v)


# A local reassigned to anything other than `this` is no longer the object, for example
# `pPool = pNode;` in a pool's Init.
#
# The whitespace after the `=` must sit inside the lookahead. Outside it, the engine backtracks
# it to zero and every `pThis = (char*)this;` reads as a rebind.
REBOUND = re.compile(r"^[ 	]*([0-9A-Za-z_]+)[ 	]*="
                     + r"(?![ 	]*\(\s*(?:const\s+)?[0-9A-Za-z_]+\s*\*\s*\)\s*this"
                     + r"(?![0-9A-Za-z_]))[ 	]*[^=;]", re.M)


def holders(body):
    return ((set(HOLDER.findall(body)) - set(MOVED.findall(body))
             - set(REBOUND.findall(body))) | set(["(char*)this"]))


def count():
    """How many byte-offset accesses into `this` are left, by the rule this tool uses.

    The holder names are read from each body by holders(), the same rule the rewrite uses.
    """
    total = 0

    for unused, text in sources((".cpp",)):
        for match in DEFINITION.finditer(text):
            end = text.find(chr(10) + "}" + chr(10), match.end())
            body = text[match.end():end if end > 0 else len(text)]

            for who in holders(body):
                total += len(re.findall(re.escape(who)
                                        + r"\s*\+\s*(?:0x[0-9a-fA-F]+|[A-Z][A-Z0-9_]{2,})",
                                        body))

    return total


# An assignment to the access, so the rewrite must keep an lvalue. `==` is not one.
ASSIGNED = re.compile(r"^\s*=(?!=)")


def shadowed(name, body):
    """True when a parameter or local has this name, so the field's name would not reach it.

    A rewrite there would change behaviour, for example `*(void**)&pOwner = pOwner;`. It is
    refused rather than qualified with `this->`. Pass the signature and the body together.
    """
    if re.search(r"[(,]\s*(?:const\s+)?[\w:*&<>]+[\s*]+" + re.escape(name) + r"\s*[,)]",
                 body):
        return True

    return re.search(r"^[ 	]+(?:[\w:*&<>]+[ 	]+)+\*?" + re.escape(name)
                     + r"[ 	]*(?:=|;|\[)", body, re.M) is not None


# Byte width of each type spelling. A type not in the table is left alone.
WIDTH = {"char": 1, "unsignedchar": 1, "signedchar": 1, "BYTE": 1,
         "short": 2, "unsignedshort": 2, "WORD": 2,
         "int": 4, "unsignedint": 4, "long": 4, "unsignedlong": 4, "float": 4,
         "DWORD": 4, "UINT": 4, "HRESULT": 4, "BOOL": 4, "LONG": 4}


def width(kind):
    if kind.endswith("*"):
        return 4

    return WIDTH.get(kind)


def recast(row, kind, raw, body, at):
    """The same BYTES under another scalar type, or None when the two are not the same width.

    The rewrite names the field and keeps the cast: `*(int*)&m_pField`. Dropping the cast would
    emit different instructions, for example when a float's bits are read as an int.
    """
    if shadowed(row[1], body):
        return None

    here = width(row[0])
    there = width(kind)

    if here is None or there is None or here != there:
        return None

    if row[0] == kind:
        return None

    return "*(%s*)&%s" % (" ".join(raw.split()), row[1])


def launder(row, kind, raw, body, at):
    """The same word through a different pointer type, or None when that is not what this is.

    This handles a pointer field read through a differently typed pointer. A read becomes
    `(U*)m_pField` and a write `*(U**)&m_pField`, because a cast expression is not an lvalue.
    Only pointer to pointer: a scalar field read as a pointer is left alone.
    """
    declared = row[0]

    if shadowed(row[1], body):
        return None

    if not declared.endswith("*") or not kind.endswith("*"):
        return None

    # The pattern consumes the cast's outer star, so `*(char**)` captures `char*` here.
    if kind.count("*") != declared.count("*"):
        return None

    # The raw spelling keeps any `const` that normalising drops.
    target = " ".join(raw.split())

    if ASSIGNED.match(body[at:]):
        return "*(%s*)&%s" % (target, row[1])

    return "(%s)%s" % (target, row[1])


# A typed address into the object, `(TransformObject*)(pThis + 0x114)`, which is `&m_field`.
# The lookbehind keeps this off the dereference form the main pass handles.
ADDRESS = (r"(?<![\w*])\(\s*((?:(?:const|volatile|unsigned|signed)\s+)*[\w:*&<>]+?)"
           + r"\s*\*\s*\)\s*\(\s*%s\s*\+\s*(0x[0-9a-fA-F]+|[A-Z][A-Z0-9_]{2,})\s*\)")


def addressOf(row, kind, raw, body):
    """`&m_field` for a typed address into the object, with the cast kept only when it differs."""
    if shadowed(row[1], body):
        return None

    if row[0] == kind:
        return "&" + row[1]

    return "(%s*)&%s" % (" ".join(raw.split()), row[1])


# An array field and its dimensions, so an access inside it becomes a subscript:
# `*(float*)(pStats + 0x8c)` on `float m_aStats[64]` at 0x000 is `m_aStats[35]`.
ARRAY = re.compile(r"^[ 	]+(?!virtual|typedef)"
                   + r"((?:(?!p(?:ublic|rotected|rivate)\s*:)[\w:*&<>])[\w:*&<>\s]*?)"
                   + r"[\s*]([0-9A-Za-z_]+)((?:\[\s*(?:0x[0-9a-fA-F]+|\d+)\s*\])+)\s*;"
                   + r"[ 	]*//\s*(0x[0-9a-fA-F]+)", re.M)

# Each dimension of a declared array, so `m_a[4][4]` gives back both fours.
DIMENSIONS = re.compile(r"\[\s*(0x[0-9a-fA-F]+|\d+)\s*\]")


def arrays():
    """{class: [(base, name, element type, element width, count)]}, own fields and bases'."""
    own = {}
    base = {}

    for unused, text in sources((".h", ".cpp")):
        for match in CLASS.finditer(text):
            if match.group(1) in own:
                continue

            base[match.group(1)] = match.group(2)
            here = []

            for kind, name, brackets, offset in ARRAY.findall(match.group(3)):
                kind = " ".join(kind.split())
                size = WIDTH.get(kind.replace(" ", ""), 4 if kind.endswith("*") else None)

                if size is None:
                    continue

                dims = [int(d, 16) if d.startswith("0x") else int(d)
                        for d in DIMENSIONS.findall(brackets)]
                total = 1

                for d in dims:
                    total *= d

                here.append((int(offset, 16), name, kind, size, total, dims))

            own[match.group(1)] = here

    found = {}

    for klass in own:
        rows = []
        at = klass
        seen = set()

        while at in own and at not in seen:
            seen.add(at)
            rows.extend(own[at])
            at = base.get(at)

        found[klass] = rows

    return found


def indexed(rows, offset, kind, raw):
    """`m_array[i]` for an access that lands on an element boundary of a field this can size.

    A multi-dimensional array gives one subscript per dimension. An element read at its own
    width under another type keeps the cast: `*(int*)&m_a[2][0]`.
    """
    for start, name, elem, size, count, dims in rows:
        if offset < start or offset >= start + size * count:
            continue

        if (offset - start) % size:
            return None

        index = (offset - start) // size
        subscripts = ""

        # Outermost dimension first; its stride is the product of the ones below it.
        for span in range(len(dims)):
            stride = 1

            for d in dims[span + 1:]:
                stride *= d

            subscripts += "[%d]" % (index // stride)
            index = index % stride

        if elem.replace(" ", "") == kind:
            return name + subscripts

        if width(elem.replace(" ", "")) == width(kind):
            return "*(%s*)&%s%s" % (" ".join(raw.split()), name, subscripts)

        return None

    return None


def main():
    if "--count" in sys.argv[1:]:
        print("%d byte-offset access(es) into `this`" % count())
        return 0

    report = "--report" in sys.argv[1:]
    launder_on = "--launder" in sys.argv[1:]
    address_on = "--address" in sys.argv[1:]
    index_on = "--index" in sys.argv[1:]
    spans = arrays()
    wanted = [a for a in sys.argv[1:] if not a.startswith("--")]

    if not wanted:
        print(__doc__)

        return 1

    known = fields()
    done = 0
    skipped = {}

    for name in sorted(sourceNames(SRC)):
        if not name.endswith(".cpp"):
            continue

        path = os.path.join(SRC, name)
        text = open(path, encoding="utf-8", errors="replace").read()
        text = text.replace(chr(13) + chr(10), chr(10))
        # This file's own offset macros, so `pObj + INTERACTOR_KIND` resolves like a literal.
        macros = dict((m, int(v, 16)) for m, v in
                      re.findall(r"^#define ([A-Z][A-Z0-9_]*) (0x[0-9a-fA-F]+)$", text, re.M))
        out = []
        at = 0

        for match in DEFINITION.finditer(text):
            klass = match.group(1)

            if klass not in wanted or klass not in known:
                continue

            end = text.find(chr(10) + "}" + chr(10), match.end())
            end = end if end > 0 else len(text)
            body = text[match.end():end]
            fresh = body

            for who in holders(body):
                spelling = re.escape(who)

                if address_on:
                    for hit in list(re.finditer(ADDRESS % spelling, fresh))[::-1]:
                        spelt = hit.group(2)
                        offset = (int(spelt, 16) if spelt.startswith("0x")
                                  else macros.get(spelt))

                        if offset is None:
                            continue

                        row = known[klass].get(offset)

                        if row is None:
                            continue

                        taken = addressOf(row, hit.group(1).replace("const", "").strip()
                                          .replace(" ", ""), hit.group(1),
                                          text[match.start():match.end()] + fresh)

                        if taken:
                            fresh = fresh[:hit.start()] + taken + fresh[hit.end():]
                            done += 1

                # The cast may carry qualifiers. `volatile` forces a reload the client makes,
                # so a volatile access keeps its cast and is skipped below.
                pattern = re.compile(r"\*\(\s*((?:(?:const|volatile|unsigned|signed)\s+)*"
                                     + r"[\w:*&<>]+?)\s*\*\s*\)\s*\(\s*" + spelling
                                     + r"\s*\+\s*(0x[0-9a-fA-F]+|[A-Z][A-Z0-9_]{2,})\s*\)")

                for hit in list(pattern.finditer(fresh))[::-1]:
                    kind = hit.group(1)

                    if "volatile" in kind:
                        skipped.setdefault("volatile", []).append(
                            "%s::%s" % (klass, match.group(2)))
                        continue

                    kind = kind.replace("const", "").strip().replace(" ", "")
                    spelt = hit.group(2)

                    if spelt.startswith("0x"):
                        offset = int(spelt, 16)
                    elif spelt in macros:
                        offset = macros[spelt]
                    else:
                        continue
                    row = known[klass].get(offset)

                    if row is None and index_on:
                        taken = indexed(spans.get(klass, ()), offset, kind, hit.group(1))

                        if taken:
                            fresh = fresh[:hit.start()] + taken + fresh[hit.end():]
                            done += 1
                            continue

                    if row is None:
                        skipped.setdefault("no field", []).append(
                            "%s::%s 0x%03x" % (klass, match.group(2), offset))
                        continue

                    if row[0] != kind:
                        laundered = launder(row, kind, hit.group(1),
                                            text[match.start():match.end()]
                                            + fresh, hit.end()
                                            + match.end() - match.start())

                        if launder_on and not laundered:
                            laundered = recast(row, kind, hit.group(1),
                                               text[match.start():match.end()] + fresh,
                                               hit.end() + match.end() - match.start())

                        if launder_on and laundered:
                            fresh = fresh[:hit.start()] + laundered + fresh[hit.end():]
                            done += 1
                            continue

                        skipped.setdefault("type differs", []).append(
                            "%s::%s 0x%03x  %s declared %s" % (klass, match.group(2), offset,
                                                               kind, row[0]))
                        continue

                    fresh = fresh[:hit.start()] + row[1] + fresh[hit.end():]
                    done += 1

            if fresh != body:
                out.append(text[at:match.end()])
                out.append(fresh)
                at = end

        if out and not report:
            out.append(text[at:])
            open(path, "w", encoding="utf-8", newline="").write("".join(out))

    for why in sorted(skipped):
        print("%s: %d" % (why, len(skipped[why])))

        for line in sorted(set(skipped[why]))[:8]:
            print("    %s" % line)

    print("%d access(es) %s" % (done, "would be rewritten" if report else "rewritten"))

    return 0


if __name__ == "__main__":
    sys.exit(main())
