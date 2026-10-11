"""Find classes that declare the same bytes as another class without deriving from it.

Two classes that describe one object, neither saying so, usually mean one is a stale copy of
the other. Evidence is the set of offsets each class declares in its `// 0xNN` field comments.
Two classes are twins when they agree over a window of offsets, declare nothing different
inside it, and neither derives from the other. Pairs rank by how many offsets they share and
how deep the deepest is.

With --subset it reports the other shape: a class whose named fields all sit at offsets another
class names, at the same widths. A gap field in one class hides the overlap from the window
rule.

    python tools/layout_twins.py              # pairs, strongest first
    python tools/layout_twins.py --min 3      # loosen the overlap the report needs
    python tools/layout_twins.py --subset     # classes whose fields are all another's
"""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

# The brace may sit a blank line below the head.
CLASS = re.compile(r"^(?:class|struct) ([0-9A-Za-z_]+)(?: *: *public ([0-9A-Za-z_]+))? *"
                   + chr(10) + r"\s*\{(.*?)" + chr(10) + r"\};", re.S | re.M)

FIELD = re.compile(r"^\s+[\w:*&<>\s]+?[\s*]([0-9A-Za-z_]+)(?:\[[^\]]*\])?;\s*//\s*(0x[0-9a-fA-F]+)",
                   re.M)

# FIELD with the declared type and array suffix kept, so a subset is checked by width too.
TYPED = re.compile(r"^\s+((?:[\w:*&<>]+\s+)*[\w:*&<>]+?)[\s*]([0-9A-Za-z_]+)(\[[^\]]*\])?;"
                   + r"\s*//\s*(0x[0-9a-fA-F]+)", re.M)

# A reserve rather than a field: it says how many bytes are undescribed, not what is there.
GAP = re.compile(r"^(?:m_)?(?:gap|reserved|undecoded|head|body|storage|block|pad)", re.I)


def sourceNames(root):
    # Sources live in subdirectories, so this walks. Names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def width(kind, array):
    """The declared width in bytes, or None when the type is not one this can size."""
    if array:
        return None

    if "*" in kind:
        return 4

    return {"char": 1, "unsignedchar": 1, "signedchar": 1,
            "short": 2, "unsignedshort": 2, "unsignedint": 4, "int": 4,
            "long": 4, "unsignedlong": 4, "float": 4, "DWORD": 4,
            "UINT": 4, "HRESULT": 4}.get(kind.replace(" ", ""))

# Below this an agreement is the ordinary head of any object: a vtable and a word or two.
SHALLOW = 0x10

# How far the agreeing window must reach. Any two classes with a run of dword fields agree at
# 0x04, 0x08 and 0x0c.
SPAN = 0x40

# How deep a subset's deepest field has to sit before the agreement means anything.
DEEP = 0x40


def classes():
    found = {}

    for name in sorted(sourceNames(SRC)):
        if not name.endswith((".cpp", ".h")):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8",
                       errors="replace").read().replace(chr(13) + chr(10), chr(10))

        for match in CLASS.finditer(text):
            if match.group(1) in found:
                continue

            offsets = {}

            for field, offset in FIELD.findall(match.group(3)):
                offsets[int(offset, 16)] = field

            if offsets:
                found[match.group(1)] = (name, match.group(2), offsets)

    return found


def related(left, right, known):
    """True when either class stands on the other, however far up the chain."""
    for start, target in ((left, right), (right, left)):
        seen = set()

        while start in known and start not in seen:
            seen.add(start)
            start = known[start][1]

            if start == target:
                return True

    return False


def typedClasses():
    """{class: ({offset: width}, file, base)} over the fields this can size."""
    found = {}

    for name in sorted(sourceNames(SRC)):
        if not name.endswith((".cpp", ".h")):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8",
                       errors="replace").read().replace(chr(13) + chr(10), chr(10))

        for match in CLASS.finditer(text):
            if match.group(1) in found:
                continue

            here = {}

            for kind, field, array, offset in TYPED.findall(match.group(3)):
                if GAP.match(field):
                    continue

                size = width(kind, array)

                if size is not None:
                    here[int(offset, 16)] = size

            if here:
                found[match.group(1)] = (here, name, match.group(2))

    return found


def subsets():
    """Classes whose every sizeable field is another class's, at the same offset and width."""
    known = typedClasses()
    plain = classes()
    rows = []

    for small in sorted(known):
        here, name, base = known[small]

        # A run of dwords at 0x0, 0x4 and 0x8 heads half the classes in the tree, so a subset
        # must reach past DEEP.
        if len(here) < 2 or max(here) < DEEP:
            continue

        for big in sorted(known):
            if big == small:
                continue

            there = known[big][0]

            if len(there) <= len(here):
                continue

            if [1 for offset in here if there.get(offset) != here[offset]]:
                continue

            if related(small, big, plain):
                continue

            rows.append((small, name, big, known[big][1], sorted(here)))

    # A class declared in a .cpp is local to one file and the easiest to fold, so those lead.
    rows.sort(key=lambda row: (not row[1].endswith(".cpp"), row[0]))

    return rows


def main():
    if "--subset" in sys.argv[1:]:
        rows = subsets()

        print("%d class(es) declare only fields another class already declares:" % len(rows))

        for small, name, big, home, offsets in rows:
            print("")
            print("    %-28s %-28s %s" % (small, name,
                                          "local to this file" if name.endswith(".cpp") else ""))
            print("    %-28s %-28s" % (big, home))
            print("        %d field(s): %s"
                  % (len(offsets), " ".join("0x%x" % offset for offset in offsets)))

        return 0

    minimum = 4

    if "--min" in sys.argv:
        minimum = int(sys.argv[sys.argv.index("--min") + 1])

    known = classes()
    names = sorted(known)
    rows = []

    for index, left in enumerate(names):
        for right in names[index + 1:]:
            shared = set(known[left][2]) & set(known[right][2])

            if len(shared) < minimum or max(shared) < SHALLOW:
                continue

            if max(shared) - min(shared) < SPAN:
                continue

            # The window must agree whole: neither class declares an offset the other does not,
            # between the first agreement and the last.
            low = min(shared)
            high = max(shared)

            def window(name):
                return set(offset for offset in known[name][2] if low <= offset <= high)

            if window(left) != window(right):
                continue

            if related(left, right, known):
                continue

            rows.append((-(len(shared) * 0x10000 + max(shared)), left, right, sorted(shared)))

    rows.sort()

    print("%d pair(s) declare the same offsets without standing on each other:" % len(rows))

    for weight, left, right, shared in rows:
        print("")
        print("    %-28s %-28s" % (left, known[left][0]))
        print("    %-28s %-28s" % (right, known[right][0]))
        print("        %d shared, deepest 0x%x: %s"
              % (len(shared), max(shared),
                 " ".join("0x%x:%s=%s" % (offset, known[left][2][offset], known[right][2][offset])
                          for offset in shared[:6])))

    return 0


if __name__ == "__main__":
    sys.exit(main())
