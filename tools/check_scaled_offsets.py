# Flag a byte offset added to a pointer that is not byte-sized.
#
# When a char* blob becomes a typed record pointer, every `p + OFFSET` written against the old
# char* silently scales by the record's size, and an outer cast hides it from the compiler.
#
# Flagged: NAME + TERM, where NAME is a pointer to something wider than a byte and TERM is a hex
# literal, a small decimal, or an upper-case offset define. A second rule flags a variable add
# read back through a scalar cast. A byte-pointer cast in front of the name,
# `(const char*)pDef + OFFSET`, is correct and is skipped.
#
# Declarations are scoped to the function that holds them; header members apply everywhere.
#
#   python tools/check_scaled_offsets.py
#
# Exit code 1 when any site is found.

import os
import re
import sys

# One star or more. A second star makes the element a pointer, so a `char**` steps by four.
DECL = re.compile(r"^\s*(?:const\s+|volatile\s+|static\s+|struct\s+|class\s+)*"
                  r"([A-Za-z_]\w*)\s*(\*+)\s*(?:const\s+)?([A-Za-z_]\w*)\s*[;,)=]")
PARAM = re.compile(r"(?:const\s+|volatile\s+|struct\s+|class\s+)*([A-Za-z_]\w*)\s*(\*+)\s*"
                   r"(?:const\s+)?([A-Za-z_]\w*)\s*[,)]")
DEFINE = re.compile(r"^\s*#define\s+([A-Z][A-Z0-9_]*)\s+(0x[0-9a-fA-F]+|\d+)\s*$")
BYTE_CAST = re.compile(r"\(\s*(?:const\s+|volatile\s+)*(?:unsigned\s+|signed\s+)?"
                       r"(?:char|void)\s*\*\s*(?:const\s*)?\)"
                       # then an optional member-access chain leading to the name
                       r"[\w\s>\-\.\(\)\*\[\]]*$")
MEMBER = re.compile(r"^\s*(?:const\s+|volatile\s+|mutable\s+|struct\s+|class\s+)*"
                    r"([A-Za-z_]\w*)\s*(\*+)\s*(m_[A-Za-z_]\w*)\s*;")


def pointee(kind, stars):
    """What one step of the pointer is: the named type for one star, a pointer for more."""
    if len(stars) > 1:
        return kind + "*"

    return kind


# The Windows byte-pointer typedefs carry their star inside the name, so `(HPSTR)*ppFormat + N`
# is a byte add that BYTE_CAST, which wants a written star, cannot see.
BYTE_TYPEDEF_CAST = re.compile(r"\(\s*(?:HPSTR|LPSTR|LPCSTR|LPBYTE|PBYTE|LPVOID)\s*\)"
                               r"[\w\s>\-\.\(\)\*\[\]]*$")

# `*(char**)&p->m_pCurrent + 0x38` puns the member's address, so the add is on the pun's type.
ADDRESS_OF = re.compile(r"&[\w\s>\-\.\(\)\*\[\]]*$")

# `*(char**)pLodNode + 6` reads a char* out of the record and adds to that, because the
# dereference binds tighter than the plus. The add is correct.
DEREF_BYTE_CAST = re.compile(r"\*\s*\(\s*(?:const\s+|volatile\s+)*"
                             r"(?:unsigned\s+|signed\s+)?(?:char|void)\s*\*\*\s*"
                             r"(?:const\s*)?\)\s*$")

# A small decimal counts too: `pCol + 8` is the same byte offset as `+ 0x08`. 0, 1, 2 and 4 are
# left out, because those are how the tree steps an array by elements.
SITE = re.compile(r"\b([A-Za-z_]\w*)\s*\+\s*"
                  r"([A-Z][A-Z0-9_]*|0x[0-9a-fA-F]+|[35-9]|[1-9]\dd{1,2})\b")

BYTE_WIDE = ("char", "void")

# The second rule. `*(unsigned char*)(pBlock + nCoord)` reads a byte at a variable displacement,
# and with pBlock typed the add scales by the record. The tell is the scalar cast: element
# arithmetic on a record is never read back through a scalar pointer. The cast may carry more
# than one star.
VARIABLE = "<a variable term>"

SCALAR_CAST_ADD = re.compile(r"\(\s*(?:const\s+|volatile\s+)*(?:unsigned\s+|signed\s+)?"
                             r"(?:char|short|int|long|float|double|void)\s*\*+\s*\)\s*\(\s*"
                             r"([A-Za-z_]\w*)\s*\+\s*[A-Za-z_]")

# A hex term on a pointer to a scalar is an element index, not a byte offset. Only record
# pointers carry the fault this looks for.
SCALARS = ("int", "short", "long", "float", "double", "unsigned", "signed", "bool",
           "wchar_t", "size_t")


def sourceNames(root):
    # Sources live in subdirectories, so this walks. Names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def tree_members(root):
    """Pointer members declared in headers, which no .cpp declares for itself.

    A name declared with two different types anywhere in the tree is dropped: this map has no
    scope, so one wrong entry would speak for every file.
    """
    seen = {}
    clash = set()

    for name in sorted(sourceNames(root)):
        if not name.endswith(".h"):
            continue

        for line in open(os.path.join(root, name), encoding="utf-8", errors="replace"):
            if line.lstrip().startswith("//"):
                continue

            m = MEMBER.match(line)
            if not m:
                continue

            kind = pointee(m.group(1), m.group(2))
            var = m.group(3)

            if var in seen and seen[var] != kind:
                clash.add(var)

            seen[var] = kind

    for var in clash:
        del seen[var]

    return seen


def tree_offsets(root):
    """{name: value} for every offset define in src/, headers included."""
    names = {}

    for name in sorted(sourceNames(root)):
        if not name.endswith((".cpp", ".h")):
            continue

        for line in open(os.path.join(root, name), encoding="utf-8", errors="replace"):
            m = DEFINE.match(line)
            if m:
                names[m.group(1)] = int(m.group(2), 0)

    return names


def scan(path, offsets, members):
    lines = open(path, encoding="utf-8", errors="replace").read().split("\n")

    pointers = dict(members)
    depth = 0
    found = []

    for n, line in enumerate(lines, 1):
        if line.lstrip().startswith("//"):
            continue

        for m in DECL.finditer(line):
            pointers[m.group(3)] = pointee(m.group(1), m.group(2))
        for m in PARAM.finditer(line):
            pointers.setdefault(m.group(3), pointee(m.group(1), m.group(2)))

        for m in SITE.finditer(line):
            name = m.group(1)
            term = m.group(2)

            if name not in pointers:
                continue
            if pointers[name] in BYTE_WIDE or pointers[name] in SCALARS:
                continue
            if not term.startswith("0x") and not term.isdigit() and term not in offsets:
                continue
            # A define worth 0, 1, 2 or 4 steps an array, the same as the bare decimal.
            if offsets.get(term) in (0, 1, 2, 4):
                continue
            if "sizeof" in line[max(0, m.start() - 12):m.start()]:
                continue
            if BYTE_CAST.search(line[:m.start()]):
                continue
            if BYTE_TYPEDEF_CAST.search(line[:m.start()]):
                continue
            if DEREF_BYTE_CAST.search(line[:m.start()]):
                continue
            if ADDRESS_OF.search(line[:m.start()]):
                continue

            found.append((n, name, pointers[name], term, line.strip()))

        for m in SCALAR_CAST_ADD.finditer(line):
            name = m.group(1)

            if name not in pointers:
                continue
            if pointers[name] in BYTE_WIDE or pointers[name] in SCALARS:
                continue
            if ADDRESS_OF.search(line[:m.start()]):
                continue
            if DEREF_BYTE_CAST.search(line[:m.start()]):
                continue

            found.append((n, name, pointers[name], VARIABLE, line.strip()))

        was_open = depth > 0
        depth += line.count("{") - line.count("}")

        # Reset only when a body that was open closes. The opening brace sits on the line after
        # the signature, so a reset at any depth of zero would drop the parameters.
        if depth <= 0:
            depth = 0

            if was_open:
                pointers = dict(members)

    return found


def main():
    root = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "src")

    offsets = tree_offsets(root)
    members = tree_members(root)

    total = 0

    for name in sorted(sourceNames(root)):
        if not name.endswith((".cpp", ".h")):
            continue

        for n, var, kind, term, text in scan(os.path.join(root, name), offsets, members):
            total += 1
            if term == VARIABLE:
                print("src/%s:%d: %s is a %s* and gains a variable term that is read"
                      " back through a scalar cast" % (name, n, var, kind))
            else:
                print("src/%s:%d: %s is a %s* and gains the byte offset %s"
                      % (name, n, var, kind, term))
            print("    %s" % text)

    print("%d byte offset(s) added to a pointer that is not byte-sized" % total)

    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
