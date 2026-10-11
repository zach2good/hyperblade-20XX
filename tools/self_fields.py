"""Name the offsets a class reaches in its own members, composites included.

Rewrites `*(T*)(pThis + 0x..)` accesses in a class's member bodies to field names. Every struct
and class in src/ is parsed, composite fields and bases are expanded, and each offset maps to a
dotted path: 0x024 on PhysicsBody is `vel.y`. Array elements and strided accesses with a runtime
index are named too.

  - The cast type matches the field: the cast goes and the field is named.
  - It does not: the cast stays and the field is named through it, `*(int*)&flSpeed`. The
    width mismatch matters to the bytes at those sites.
  - `volatile` always stays. It forces a reload where the client reloads.
  - A field whose name a local or a parameter already uses is refused. Naming it would write
    the local.

    python tools/self_fields.py --report CameraObject     # say what it would do
    python tools/self_fields.py CameraObject Radar        # rewrite these classes
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

RECORD = re.compile(r"^(?:class|struct) ([0-9A-Za-z_]+)(?: *: *public ([0-9A-Za-z_]+))? *"
                    + chr(10) + r"\{(.*?)" + chr(10) + r"\};", re.S | re.M)
FIELD = re.compile(r"^\s+((?:[\w:*&<>]+\s+)*[\w:*&<>]+?)[\s*]([0-9A-Za-z_]+)(\[[^\]]*\])?;"
                   + r"\s*//\s*(0x[0-9a-fA-F]+)", re.M)
DEFINE = re.compile(r"^#define ([0-9A-Za-z_]+) (0x[0-9a-fA-F]+)$", re.M)

# A four-byte scalar declared with no offset comment, which is how Vec3 spells x, y and z.
PLAIN = re.compile(r"^ {4}(float|int|unsigned int|short|char) ([0-9A-Za-z_]+);\s*$", re.M)

LOCAL = re.compile(r"^\s+(?:const |volatile |unsigned |signed )*"
                   + r"[0-9A-Za-z_]+[\s*]+([0-9A-Za-z_]+)\s*(?:;|\[|=)", re.M)
PARAMETER = re.compile(r"[0-9A-Za-z_]+[\s*]+([0-9A-Za-z_]+)\s*(?:,|$)")

# Either spelling of the alias: the declaration with its initialiser, and
# the bare assignment further down that src/ui/screens/teamsetupuiscreen.cpp uses.
HOLDER = re.compile(r"^[ \t]+(?:char\* (?:const )?)?([0-9A-Za-z_]+) = \(char\*\)this;"
                    + chr(10), re.M)


# Records windows.h declares, so src/ does not.
BUILTIN = {
    "RECT": (None, [(0, "int", "left", ""), (4, "int", "top", ""),
                    (8, "int", "right", ""), (12, "int", "bottom", "")]),
    "POINT": (None, [(0, "int", "x", ""), (4, "int", "y", "")]),
    "SIZE": (None, [(0, "int", "cx", ""), (4, "int", "cy", "")]),
}


def sourceNames(root):
    # Sources live in subdirectories, so this walks. Names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def records():
    """{name: (base, [(offset, type, field, array)])} for every record in src/."""
    found = dict(BUILTIN)

    for name in sorted(sourceNames(SRC)):
        if not name.endswith((".cpp", ".h")):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8",
                       errors="replace").read().replace(chr(13) + chr(10), chr(10))

        for match in RECORD.finditer(text):
            if match.group(1) in found:
                continue

            fields = []

            for kind, field, array, offset in FIELD.findall(match.group(3)):
                kind = re.sub(r"^(?:class|struct) ", "", kind.strip())
                fields.append((int(offset, 16), kind, field, array))

            # A small record such as Vec3 carries no `// 0x` comments, so its offsets come from
            # declaration order.
            if not fields:
                fields = PLAIN.findall(match.group(3))
                fields = ([(index * 4, kind, field, "")
                           for index, (kind, field) in enumerate(fields)]
                          if fields and len(fields) * 4 <= 16 else [])

            found[match.group(1)] = (match.group(2), fields)

    return found


# The element widths an array can be walked with. A pointer is four whatever it points at.
# `char` is absent: every char array in this tree is a gap or a reserve, and a word inside one
# reads worse by name than by offset.
ELEMENT = {"int": 4, "unsigned int": 4, "long": 4, "unsigned long": 4, "DWORD": 4, "float": 4,
           "short": 2, "unsigned short": 2}


def stride(kind):
    """The bytes one element of `kind` occupies, or 0 when this cannot say."""
    if kind.endswith("*"):
        return 4

    return ELEMENT.get(kind, 0)


def bound(array):
    """The element count out of a `[255]` or `[0x04]` suffix, or 0 when it has none."""
    try:
        return int(array.strip("[]"), 0)
    except ValueError:
        return 0


def arrays(klass, known, seen=None):
    """{offset: (path, element type, stride, count)} for the array members, base included.

    A strided access has a runtime index, so it needs the array itself:
    `pObj + nIndex * 4 + 0x08` is m_apSlots[nIndex].
    """
    if klass not in known or (seen and klass in seen):
        return {}

    seen = (seen or set()) | set([klass])
    base, fields = known[klass]
    found = dict(arrays(base, known, seen))

    for offset, kind, field, array in fields:
        if array and stride(kind) and bound(array):
            found[offset] = (field, kind, stride(kind), bound(array))

    return found


def layout(klass, known, seen=None):
    """{offset: (dotted path, type)} for a class, its base, and the composites inside it."""
    if klass not in known or (seen and klass in seen):
        return {}

    seen = (seen or set()) | set([klass])
    base, fields = known[klass]
    found = dict(layout(base, known, seen))

    for offset, kind, field, array in fields:
        if not array and kind in known:
            for at, pair in layout(kind, known, seen).items():
                found[offset + at] = ("%s.%s" % (field, pair[0]), pair[1])

        # An offset inside an array is an element of it. A later field overwrites whatever the
        # array claimed past its own end, because fields are walked in declaration order.
        if array and stride(kind):
            for index in range(1, bound(array)):
                found[offset + index * stride(kind)] = ("%s[%d]" % (field, index), kind)

        found[offset] = (field + "[0]" if array else field, kind)

    return found


def members(klass):
    """Every out-of-line definition of one class, constructors and destructors included.

    The return type is optional because a constructor has none, and an initialiser list may sit
    between the parameters and the brace.
    """
    return re.compile(r"^(?:[\w:*&<>\s]+?\b)?%s::[~0-9A-Za-z_]+\s*\(([^;{]*)\)\s*"
                      % re.escape(klass) + r"(?:const\s*)?(?::[^;{]*)?\s*\{", re.M)


def term(word, macros):
    """One offset term as a number: a macro, a hex literal, or a decimal one."""
    if word in macros:
        return macros[word]

    if re.match(r"^0[xX][0-9a-fA-F]+$", word):
        return int(word, 16)

    if word.isdigit():
        return int(word)

    return None


def total(expr, macros):
    """An offset spelled as a sum of terms, such as `CAMERA_SMOOTH_VEL + 8`, or None."""
    out = 0

    for word in expr.split("+"):
        value = term(word.strip(), macros)

        if value is None:
            return None

        out += value

    return out


HEADER_MACROS = None


def headerMacros():
    """The offset defines in headers, which a source file gets by including one.

    A name two headers define differently is dropped, because a wrong offset names the wrong
    field silently. A source file's own defines win over these.
    """
    found = {}
    clash = set()

    for name in sorted(sourceNames(SRC)):
        if not name.endswith(".h"):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8").read()

        for key, value in DEFINE.findall(text):
            here = int(value, 16)

            if key in found and found[key] != here:
                clash.add(key)

            found[key] = here

    for key in clash:
        del found[key]

    return found


# The blob cast spelled out at each site rather than taken into a local, which HOLDER misses.
INLINE = r"\(\s*char\s*\*\s*\)\s*this"


def rewrite(klass, fields, blocks, report, missed):
    """Every member of one class, in every file that defines one. Answers how many were named."""
    global HEADER_MACROS

    if HEADER_MACROS is None:
        HEADER_MACROS = headerMacros()

    pattern = members(klass)
    done = 0

    for name in sorted(sourceNames(SRC)):
        if not name.endswith(".cpp"):
            continue

        path = os.path.join(SRC, name)
        text = open(path, encoding="utf-8").read().replace(chr(13) + chr(10), chr(10))

        if klass + "::" not in text:
            continue

        macros = dict(HEADER_MACROS)
        macros.update(dict((key, int(value, 16)) for key, value in DEFINE.findall(text)))
        out = []
        at = 0

        for match in pattern.finditer(text):
            end = text.find(chr(10) + "}" + chr(10), match.end())

            if end < 0 or match.start() < at:
                continue

            body = text[match.end():end]
            holder = HOLDER.search(body)

            if holder is None:
                local = None
                base = INLINE
            else:
                local = holder.group(1)
                base = re.escape(local)

            shadowed = set(LOCAL.findall(body)) | set(PARAMETER.findall(match.group(1)))
            access = re.compile(r"(\*?)\(\s*((?:const |volatile |unsigned |signed )*)"
                                + r"([0-9A-Za-z_]+)\s*(\*+)\s*\)\s*\(\s*%s\s*\+\s*"
                                % base
                                + r"([0-9A-Za-z_]+(?:\s*\+\s*[0-9A-Za-z_]+)*)\s*\)")

            # Count the rewrites themselves: subn would count matches swap leaves alone.
            named = [0]

            def swap(hit):
                offset = total(hit.group(5), macros)

                if offset is None or offset not in fields:
                    missed.append((klass, name, hit.group(0)[:58]))

                    return hit.group(0)

                field = fields[offset][0]
                kind = fields[offset][1]

                if field.split(".")[0].replace("[0]", "") in shadowed:
                    missed.append((klass, name, hit.group(0)[:46] + "  shadowed"))

                    return hit.group(0)

                stars = hit.group(4)
                # The sign qualifier is part of the type: `unsigned char*` must not match a field
                # declared `char*`.
                qualifier = hit.group(2).replace("volatile ", "")
                claimed = qualifier + hit.group(3) + "*" * (len(stars) - 1)

                named[0] += 1

                if claimed == kind and "volatile" not in hit.group(2):
                    return field if hit.group(1) == "*" else "&" + field

                return "%s(%s%s%s)&%s" % (hit.group(1), hit.group(2), hit.group(3), stars, field)

            # `local + <index> * <stride> + <base>`, and the same with the two the other way
            # round: an element of an array whose index is only known at run time.
            strided = re.compile(r"(\*?)\(\s*((?:const |volatile |unsigned |signed )*)"
                                 + r"([0-9A-Za-z_]+)\s*(\*+)\s*\)\s*\(\s*%s\s*\+\s*"
                                 % base
                                 + r"(?:([0-9A-Za-z_]+)\s*\*\s*([0-9A-Za-z_]+)\s*\+\s*"
                                 + r"([0-9A-Za-z_]+)|([0-9A-Za-z_]+)\s*\+\s*"
                                 + r"([0-9A-Za-z_]+)\s*\*\s*([0-9A-Za-z_]+))\s*\)")

            def swapStrided(hit):
                if hit.group(5) is not None:
                    index = hit.group(5)
                    step = term(hit.group(6), macros)
                    offset = term(hit.group(7), macros)
                else:
                    offset = term(hit.group(8), macros)
                    index = hit.group(9)
                    step = term(hit.group(10), macros)

                if offset is None or step is None or offset not in blocks:
                    return hit.group(0)

                field, kind, width, count = blocks[offset]

                # The stride must be the element's width. A walk in another unit over the same
                # base is a different array.
                if step != width or field in shadowed:
                    return hit.group(0)

                named[0] += 1
                stars = hit.group(4)
                qualifier = hit.group(2).replace("volatile ", "")
                claimed = qualifier + hit.group(3) + "*" * (len(stars) - 1)
                element = "%s[%s]" % (field, index)

                if claimed == kind and "volatile" not in hit.group(2):
                    return element if hit.group(1) == "*" else "&" + element

                return "%s(%s%s%s)&%s" % (hit.group(1), hit.group(2), hit.group(3), stars,
                                          element)

            fixed = strided.sub(swapStrided, body)
            fixed = access.sub(swap, fixed)
            count = named[0]
            named[0] = 0

            if not count:
                continue

            if holder is not None and not re.search(
                    r"(?<![0-9A-Za-z_])%s(?![0-9A-Za-z_])" % re.escape(local),
                    fixed[holder.end():]):
                fixed = fixed[:holder.start()] + fixed[holder.end():]

            done += count
            out.append(text[at:match.end()])
            out.append(fixed)
            at = end

        out.append(text[at:])
        new = "".join(out)

        if new != text and not report:
            open(path, "w", encoding="utf-8", newline="").write(new)

    return done


def main():
    report = "--report" in sys.argv[1:]
    wanted = [name for name in sys.argv[1:] if not name.startswith("--")]

    if not wanted:
        print(__doc__)

        return 1

    known = records()
    done = 0
    missed = []

    for klass in wanted:
        fields = layout(klass, known)

        if not fields:
            print("%s declares no fields this can read" % klass)
            continue

        done += rewrite(klass, fields, arrays(klass, known), report, missed)

    print("%d access(es) named, %d this cannot place" % (done, len(missed)))

    for row in missed:
        print("    %-22s %-28s %s" % row)

    return 0


if __name__ == "__main__":
    sys.exit(main())
