"""Say which slots a class overrides, and who owns each overriding body today.

A `g_vtbl` array is a class the tree has not declared. This diffs each table against its base's
and names the body at each differing word, with the file that marks it and how many tables hold
it. The output is a worklist for declaring the derived class.

An override keeps the base's slot name and exact signature. A
renamed override is a new slot in VC6, and the class then declares more slots than its table.

    python tools/slot_owners.py 0x004b1744           # every pool table against this base
    python tools/slot_owners.py 0x004b1744 0x004b1c00  # just this one

The base's slot count comes from symbols/vtablearity.h, which reads it out of the image.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import image_tables as g

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")


def sourceNames(root):
    # Paths come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def slotCount(address):
    """How many slots the image gives a table, from the generated header."""
    text = open(os.path.join(ROOT, "symbols", "vtablearity.h"), encoding="utf-8",
                   errors="replace").read()
    hit = re.search(r"#define VTABLE_SLOTS_%08x (\d+)" % address, text)

    if hit:
        return int(hit.group(1))

    return 0


def poolTables():
    """Every table a pool stores into the node it builds."""
    found = set()

    for name in sorted(sourceNames(SRC)):
        if not name.startswith("pool") or not name.endswith(".cpp"):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8", errors="replace").read()

        for hit in re.findall(r"g_vtbl(004[bB][0-9a-fA-F]{4})", text):
            found.add(int("0x" + hit, 16))

    return sorted(found)


# A marked body and its signature, which may wrap, up to the opening brace.
BODY = re.compile(r"// FUNCTION: HYPERX (0x00[0-9a-fA-F]{6})" + chr(10)
                  + r"([^" + chr(10) + r"]*" + chr(10) + r"?[^{]*)\{")


def owners():
    found = {}

    for name in sorted(sourceNames(SRC)):
        if not name.endswith(".cpp"):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8", errors="replace").read()
        text = text.replace(chr(13) + chr(10), chr(10))

        for hit in BODY.finditer(text):
            found[int(hit.group(1), 16)] = (name, " ".join(hit.group(2).split())[:70])

    return found


def holders(image, count=40):
    """{body address: the tables that hold it}, over every table the tree names.

    `count` is an upper bound. The real bound is the next table's address, applied below.

    A body in one table belongs to that table's class and can be renamed onto it. A body in
    many tables is linker folding, and only one class can carry its marker.
    """
    found = {}
    tables = sorted(set(g.knownTables()) | set(poolTables()))

    for position, start in enumerate(tables):
        # A table ends where the next one begins, so a short table does not read its
        # neighbour's words.
        limit = start + count * 4

        if position + 1 < len(tables):
            limit = min(limit, tables[position + 1])

        at = start

        while at < limit:
            word = image.word(at)

            if word is None or not image.isCode(word):
                break

            found.setdefault(word, set()).add(start)
            at += 4

    return found


def main():
    arguments = [a for a in sys.argv[1:] if a.startswith("0x")]

    if not arguments:
        print(__doc__)
        return 2

    base = int(arguments[0], 16)
    count = slotCount(base)

    if count == 0:
        print("0x%08x has no slot count in symbols/vtablearity.h" % base)
        return 1

    image = g.Image(g.referencePath())
    known = owners()
    rows = [int(a, 16) for a in arguments[1:]] or poolTables()

    baseSlots = [image.word(base + i * 4) for i in range(count)]
    shared = holders(image, count)

    print("base 0x%08x, %d slot(s); %d table(s) to read" % (base, count, len(rows)))

    for start in rows:
        if start == base:
            continue

        here = [image.word(start + i * 4) for i in range(count)]
        differ = [i for i in range(count) if here[i] != baseSlots[i]]

        print("")
        print("0x%08x  %d override(s): %s"
              % (start, len(differ), " ".join(str(i) for i in differ)))

        for index in differ:
            who = known.get(here[index])

            print("    slot %2d  0x%08x  in %2d table(s)  %s"
                  % (index, here[index], len(shared.get(here[index], ())),
                     "%s  %s" % who if who else "NOT IN THE TREE"))

    return 0


if __name__ == "__main__":
    sys.exit(main())
