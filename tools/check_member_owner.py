"""Fail when a member function sits on a class the image's tables do not put it on.

A marker above `X::Method` claims the address is X's. A body is often named after what it does,
which is not always the class whose table holds it; a destructor that destroys a transform is
not necessarily TransformObject's. The table is the evidence.

The tool reads every .rdata word of the reference image and asks which tables hold each marked
address. A table runs until the next table the tree names or the first word that is not a .text
address. A class pinned with `// VTABLE:` or `// SLOT TABLE` is checked against its tables; a
class anywhere on the inheritance chain between the two counts as the owner, since the image
folds base bodies into derived tables. An address in no named table is passed over.

A free function whose address sits in a named class's table is reported too: it is that class's
slot. symbols/free_slots.csv lists addresses that stay where they are, with the reason, and
the checker passes over them.

    python tools/check_member_owner.py

Exit code 1 when a marked member is on a class that owns none of the named tables holding it,
or when a marked free function sits in a named class's table without a row in that register.
"""
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import image_tables as image

SRC = os.path.join(image.ROOT, "src")

FREE = re.compile(r"// FUNCTION: HYPERX (0x00[0-9a-f]{6})" + chr(10)
                  + r"(?:[^" + chr(10) + r"]*" + chr(10) + r")?"
                  + r"[\w:*&<>\s]*?" + chr(92) + r"b([0-9A-Za-z_]+)" + chr(92) + r"s*"
                  + chr(92) + r"([^;{]*" + chr(92) + r")" + chr(92) + r"s*" + chr(10)
                  + r"?" + chr(92) + r"{")

BODY = re.compile(r"// FUNCTION: HYPERX (0x00[0-9a-f]{6})" + chr(10)
                  + r"(?:[^" + chr(10) + r"]*" + chr(10) + r")?"
                  + r"[\w:*&<>\s]*?" + chr(92) + r"b([0-9A-Za-z_]+)::([~0-9A-Za-z_]+)" + chr(92)
                  + r"s*" + chr(92) + r"(")

MARKER = re.compile(r"// (?:VTABLE: HYPERX|SLOT TABLE) (0x00[0-9a-fA-F]{6})[^" + chr(10) + r"]*"
                    + chr(10) + r"(?:class|struct) ([0-9A-Za-z_]+)")

BASE_OF = re.compile("^(?:class|struct) ([0-9A-Za-z_]+) *: *public ([0-9A-Za-z_]+)", re.M)


def sourceNames(root):
    # Sources live in subdirectories, so walk rather than list. Names are relative to root.
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


def bodies():
    found = []

    for name, text in sources((".cpp",)):
        for match in BODY.finditer(text):
            found.append((int(match.group(1), 16), match.group(2), name, match.group(3)))

    return found


def freeBodies():
    """Every marked free function: the ones with no `Klass::` before the name."""
    found = []

    for name, text in sources((".cpp",)):
        for match in FREE.finditer(text):
            # Check the whole signature: in a two-line one the `::` can sit on the first line.
            head = text[match.start():match.end()]

            if "::" in head[head.index(chr(10)) + 1:]:
                continue

            found.append((int(match.group(1), 16), match.group(2), name))

    return found


def allowed():
    """Addresses symbols/free_slots.csv accepts as free despite sitting in a named table."""
    path = os.path.join(image.ROOT, "symbols", "free_slots.csv")

    if not os.path.exists(path):
        return {}

    found = {}

    for line in open(path, encoding="utf-8").read().splitlines():
        if not line.strip() or line.startswith("#") or line.startswith("address"):
            continue

        parts = line.split(",", 2)
        found[int(parts[0], 16)] = parts[2] if len(parts) > 2 else ""

    return found


def pinned():
    found = {}

    for unused, text in sources((".h", ".cpp")):
        for match in MARKER.finditer(text):
            found[int(match.group(1), 16)] = match.group(2)

    return found


def bases():
    found = {}

    for unused, text in sources((".h", ".cpp")):
        for derived, base in BASE_OF.findall(text):
            found[derived] = base

    return found


def ancestry(klass, known):
    found = set([klass])

    while klass in known:
        klass = known[klass]

        if klass in found:
            break

        found.add(klass)

    return found


def tableExtents(starts, reference):
    """{table start: the address one past its last slot}.

    A table runs until the next table any part of the tree names, pinned or not, or until a
    word that is not a .text address. Stopping only at pinned tables would let a table swallow
    the unpinned ones after it.
    """
    found = {}
    bounds = sorted(set(starts) | set(image.knownTables()))

    for start in sorted(starts):
        limit = start + 0x400

        for other in bounds:
            if start < other < limit:
                limit = other

        at = start

        while at < limit:
            word = reference.word(at)

            if word is None or not reference.isCode(word):
                break

            at += 4

        found[start] = at

    return found


def tablesHolding(addresses, starts=None):
    """{address: [every table that holds it]}, or the word's own address when nothing is
    named. A body can sit in several tables at once, which is what folding looks like."""
    found = {}
    reference = image.Image(image.referencePath())
    extents = tableExtents(starts or [], reference)

    for name, start, size, raw in reference.sections:
        if name != ".rdata":
            continue

        for offset in range(0, size - 4, 4):
            if raw + offset + 4 > len(reference.data):
                break

            word = struct.unpack_from("<I", reference.data, raw + offset)[0]

            if word not in addresses:
                continue

            at = start + offset
            covering = [table for table, end in extents.items() if table <= at < end]

            found.setdefault(word, []).extend(covering or [at])

    return found


def main():
    found = bodies()
    free = freeBodies()
    known = pinned()
    derived = bases()
    accepted = allowed()

    where = tablesHolding(set(row[0] for row in found) | set(row[0] for row in free),
                          known)
    wrong = []
    unpinned = 0

    for address, klass, name, method in sorted(found):
        owners = [known.get(table) for table in where.get(address, [])]

        if address in accepted:
            continue

        if not [owner for owner in owners if owner]:
            unpinned += 1
            continue

        # A base's table is the derived class's too where the image folded them, so a class
        # anywhere on the chain between the two counts as owning it.
        if [owner for owner in owners if owner and klass in ancestry(owner, derived)]:
            continue

        if [owner for owner in owners if owner and owner in ancestry(klass, derived)]:
            continue

        wrong.append((address, klass, method, name, where.get(address, []), owners))

    for address, klass, method, name, tables, owners in wrong:
        print("%s  0x%08x is %s::%s, but its table%s"
              % (name, address, klass, method, "s are" if len(tables) > 1 else " is"))

        for table, owner in zip(tables, owners):
            print("        0x%08x  %s" % (table, owner or "unnamed"))

    loose = []

    for address, name, home in sorted(free):
        if address in accepted:
            continue

        owners = [known.get(table) for table in where.get(address, [])]

        if [owner for owner in owners if owner]:
            loose.append((address, name, home,
                          sorted(set(owner for owner in owners if owner))))

    for address, name, home, owners in loose:
        print("%s  0x%08x is the free function %s, but it is a slot of %s"
              % (home, address, name, " and ".join(owners)))

    print("%d marked member%s, %d in no named table, %d on the wrong class"
          % (len(found), "" if len(found) == 1 else "s", unpinned, len(wrong)))
    print("%d marked free function%s, %d accepted by symbols/free_slots.csv, %d in a named table"
          % (len(free), "" if len(free) == 1 else "s", len(accepted), len(loose)))

    if wrong:
        print("The table that holds the address names the class, not what the body does.")

    if loose:
        print("Make each a member of the class whose table holds it, or give it a row in")
        print("symbols/free_slots.csv saying what blocks that.")

    if wrong or loose:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
