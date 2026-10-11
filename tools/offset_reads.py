# Classify `*(T*)((char*)p + N)` reads, the blob's shape on a pointer that is not `this`.
#
# Where a class covers the offset, the read is a member access and the cast can go. Where nothing
# covers it, the site stays until a decode happens. Each site gets one kind:
#
#   TYPED     a pointer type is written for the base and that type covers the offset. Convert.
#   PAST      a type is written and the offset is at or beyond its STATIC_ASSERT size, so it
#             lands in a subclass's undecoded run. Naming it would pick one subclass.
#   STEP      a type is written and the offset is not a constant, so no field can name it.
#   BLOB      the base is a char*, a void* or an int, or a call or a subscript whose type the
#             site does not write. The decode is the work, not the spelling.
#   LIBRARY   inside src/jpeg or src/common/crt, which keeps its own spelling.
#
# A class with no STATIC_ASSERT is never called PAST. The cast may be to a const or unsigned
# char, and the site may break across lines anywhere.
#
#     python tools/offset_reads.py           # totals only
#     python tools/offset_reads.py --list    # every site
#
# Read only.

import collections
import os
import re
import sys

import comments

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Only the head is a pattern. The base and the offset are walked by bracket depth, because a base
# can be a call or a subscript and the `+` can fall on the next line.
SITE = re.compile(r"\*\(\s*(?:const\s+)?[\w ]+?\s*\*+\s*\)\s*\(\s*\(\s*(?:const\s+)?"
                  r"(?:unsigned\s+)?char\s*\*\s*\)")
DECL = re.compile(r"(?:const\s+|volatile\s+|static\s+|struct\s+|class\s+)*"
                  r"([\w:]+)\s*\*+\s*(?:const\s+)?(\w+)\s*[;,)=]")
BLOB = ("char", "void", "int", "unsigned", "short", "long", "float", "BYTE", "WORD", "DWORD")
LIBRARY = ("jpeg/", "common/crt")

# Each decoded class's size, from its STATIC_ASSERT.
SIZE = re.compile(r"STATIC_ASSERT\(\s*sizeof\((\w+)\)\s*==\s*(0x[0-9a-fA-F]+|\d+)\s*\)")
DEFINE = re.compile(r"^\s*#\s*define\s+(\w+)\s+(0x[0-9a-fA-F]+|\d+)\s*$", re.M)
NAME = re.compile(r"[A-Za-z_]\w*")


def sizes():
    """Every class whose size this tree asserts, which is what makes PAST answerable."""
    found = {}

    for directory, unused, names in os.walk("src"):
        for name in sorted(names):
            if not name.endswith((".cpp", ".h")):
                continue

            text = open(os.path.join(directory, name), encoding="utf-8",
                           errors="replace").read()

            for who, value in SIZE.findall(text):
                found[who] = int(value, 0)

    return found


def base(code, at):
    """The base after the char cast, and where its `+` ends; None when no `+` follows it."""
    depth = 0
    out = []

    for index in range(at, min(len(code), at + 400)):
        ch = code[index]

        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1

            if depth < 0:
                return None
        elif depth == 0 and ch == "+":
            return " ".join("".join(out).split()), index + 1
        elif depth == 0 and ch in ",;?=":
            return None
        elif depth == 0 and ch == "-" and code[index + 1:index + 2] != ">":
            return None

        out.append(ch)

    return None


def offset(line, at):
    """The offset expression of one site, from the `+` to the paren that closes the base."""
    depth = 1
    out = []

    for ch in line[at:]:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1

            if depth == 0:
                break

        out.append(ch)

    return "".join(out).strip()


def resolve(term, defines):
    """The offset as a number, or None when a variable or a member is in it."""
    # Hex literals become numbers first, so the name pattern cannot read `0xd20` as `xd20`.
    filled = re.sub(r"\b0x[0-9a-fA-F]+\b", lambda m: str(int(m.group(0), 16)), term)

    for name in set(NAME.findall(filled)):
        if name not in defines:
            return None

        filled = re.sub(r"\b%s\b" % name, str(defines[name]), filled)

    if re.search(r"[^0-9+\-*/ ()]", filled):
        return None

    try:
        return int(eval(filled, {"__builtins__": {}}, {}))
    except Exception:
        return None


def sources(root):
    found = []

    for directory, unused, names in os.walk(root):
        for name in sorted(names):
            if name.endswith(".cpp"):
                found.append(os.path.join(directory, name))

    return found


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    typed = []
    past = []
    computed = []
    blob = collections.Counter()
    library = 0

    known = sizes()

    for path in sources("src"):
        shown = os.path.relpath(path, "src").replace(os.sep, "/")
        text = open(path, encoding="utf-8", errors="replace").read()
        code = comments.code(text)
        declared = {name: kind for kind, name in DECL.findall(code)}
        defines = {name: int(value, 0) for name, value in DEFINE.findall(code)}

        for found in SITE.finditer(code):
            walked = base(code, found.end())

            if walked is None:
                continue

            at = code.count("\n", 0, found.start()) + 1
            base_text, plus = walked

            if any(mark in shown for mark in LIBRARY):
                library += 1
                continue

            # The base may be a member chain; the last name in it is what carries a type. A call or
            # a subscript leaves no name, and so reads as a blob.
            name = re.split(r"[>.\[\]()\-]+", base_text)[-1] or base_text
            kind = declared.get(name)

            if not kind or kind in BLOB:
                blob[shown] += 1
                continue

            term = " ".join(offset(code, plus).split())
            value = resolve(term, defines)
            size = known.get(kind.split("::")[-1])

            if value is None:
                computed.append((shown, at, base_text, kind))
                continue

            if size is not None and value >= size:
                past.append((shown, at, base_text, kind, size, hex(value)))
                continue

            typed.append((shown, at, base_text, kind))

    if show:
        for shown, at, base_text, kind in typed:
            print("TYPED  %-40s %-5d %-26s declared %s*" % (shown, at, base_text, kind))

        for shown, at, base_text, kind, size, where in past:
            print("PAST   %-40s %-5d %-26s %s is %s and the step is %s"
                  % (shown, at, base_text, kind, hex(size), where))

        for shown, at, base_text, kind in computed:
            print("STEP   %-40s %-5d %-26s declared %s*, the step is computed"
                  % (shown, at, base_text, kind))

        for shown, n in blob.most_common(12):
            print("BLOB   %-40s %d site(s)" % (shown, n))

    print("%d offset read(s): %d the written type covers, %d past the end of the type that is "
          "written, %d stepped by a value rather than a constant, %d on a blob, %d in "
          "library code"
          % (len(typed) + len(past) + len(computed) + sum(blob.values()) + library,
             len(typed), len(past), len(computed), sum(blob.values()), library))

    return 0


if __name__ == "__main__":
    sys.exit(main())
