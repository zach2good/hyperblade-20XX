# Find char* locals assigned from something whose type is already declared.
#
#     python tools/check_discarded_types.py           # count
#     python tools/check_discarded_types.py --list    # every site
#
# tools/check_named_offsets.py cannot resolve a base whose type is not written down. A local
# declared `char*` and handed a typed pointer throws the type away, and every `+ 0xNN` after it
# is invisible to that tool.
#
# Reported: a local declared `char*` or `void*`, assigned `(char*)EXPR` where EXPR ends in a
# member or a method call whose declared type is a pointer to a class. A name declared with two
# different types in the headers is dropped.
#
# A hit says the type is knowable, not that the conversion costs no bytes. Always exits 0.

import os
import re
import sys

MEMBER = re.compile(r"^\s*(?:const\s+|volatile\s+|mutable\s+|struct\s+|class\s+)*"
                    r"([A-Za-z_]\w*)\s*\*\s*(m_[A-Za-z_]\w*)\s*;")
LOCAL = re.compile(r"^\s*(?:const\s+)?(?:char|void)\s*\*\s*(?:const\s+)?([A-Za-z_]\w*)\s*;")
# The access chain is optional, so a bare `(char*)m_pAttach` on `this` matches too.
ASSIGN = re.compile(r"\b([A-Za-z_]\w*)\s*=\s*\(\s*(?:const\s+)?(?:char|void)\s*\*\s*\)\s*"
                    r"((?:[A-Za-z_][\w>\-\.]*)?\b(m_[A-Za-z_]\w*))\s*;")

CALL = re.compile(r"\b([A-Za-z_]\w*)\s*=\s*\(\s*(?:const\s+)?(?:char|void)\s*\*\s*\)\s*"
                  r"([A-Za-z_][\w>\-\.]*\b([A-Za-z_]\w*))\s*\(\s*\)\s*;")

BYTE_WIDE = ("char", "void")


def sourceNames(root):
    # Sources live in subdirectories. Names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def tree_members(root):
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

            kind, var = m.group(1), m.group(2)

            if var in seen and seen[var] != kind:
                clash.add(var)

            seen[var] = kind

    for var in clash:
        del seen[var]

    return seen



# Method return types, for sites such as `= (char*)pContext->GetActor();`. A name declared with
# two different return types is dropped.
METHOD = re.compile(r"^\s*(?:virtual\s+)?(?:const\s+|struct\s+|class\s+)*"
                    r"([A-Za-z_]\w*)\s*\*\s*([A-Za-z_]\w*)\s*\([^;{]*\)\s*(?:const\s*)?[;={]")


def tree_methods(root):
    seen = {}
    clash = set()

    for name in sorted(sourceNames(root)):
        if not name.endswith(".h"):
            continue

        for line in open(os.path.join(root, name), encoding="utf-8", errors="replace"):
            if line.lstrip().startswith("//"):
                continue

            m = METHOD.match(line)
            if not m:
                continue

            kind, fn = m.group(1), m.group(2)

            if kind in ("return", "if", "while", "for", "switch"):
                continue

            if fn in seen and seen[fn] != kind:
                clash.add(fn)

            seen[fn] = kind

    for fn in clash:
        del seen[fn]

    return seen

def scan(path, members, methods):
    lines = open(path, encoding="utf-8", errors="replace").read().split("\n")

    locals_seen = set()
    found = []

    for line in lines:
        if line.lstrip().startswith("//"):
            continue
        m = LOCAL.match(line)
        if m:
            locals_seen.add(m.group(1))

    for n, line in enumerate(lines, 1):
        if line.lstrip().startswith("//"):
            continue

        for m in ASSIGN.finditer(line):
            target, source, member = m.group(1), m.group(2), m.group(3)

            if target not in locals_seen:
                continue
            if member not in members:
                continue
            if members[member] in BYTE_WIDE:
                continue

            found.append((n, target, members[member], source, line.strip()))

        for m in CALL.finditer(line):
            target, source, fn = m.group(1), m.group(2), m.group(3)

            if target not in locals_seen:
                continue
            if fn not in methods:
                continue
            if methods[fn] in BYTE_WIDE:
                continue

            found.append((n, target, methods[fn], source, line.strip()))

    return found


def main():
    root = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "src")
    show = "--list" in sys.argv

    members = tree_members(root)
    methods = tree_methods(root)

    total = 0
    files = set()

    for name in sorted(sourceNames(root)):
        if not name.endswith(".cpp"):
            continue

        for n, target, kind, source, text in scan(os.path.join(root, name), members, methods):
            total += 1
            files.add(name)

            if show:
                print("src/%s:%d: %s is a char* holding a %s*" % (name, n, target, kind))
                print("    %s" % text[:110])

    print("%d char* local(s) in %d file(s) assigned a pointer whose type is already declared"
          % (total, len(files)))

    return 0


if __name__ == "__main__":
    sys.exit(main())
