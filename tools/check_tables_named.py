"""Fail when a vtable in the image has no class in this tree.

Walks the original's .rdata, finds the runs of consecutive pointers into .text, and reports any
run that no `// VTABLE:` or `// SLOT TABLE` marker covers. A pinned table's span runs from its
own address to the next pinned one, stopping early at the first word that is not code, so
adjacent tables tile. Every vtable in HYPERX.EXE has a class, so a report means a table the tree
has stopped naming.

The threshold is three consecutive words. The image's .rdata holds hundreds of lone pointers and
pairs, from jump tables, plan opcode rows and the CRT, so two would report false runs. A two slot
vtable slips past: InfoBarSlot's at 0x004b3350 is one, and it is pinned.

    python tools/check_tables_named.py

Exit code 1 when a run is uncovered.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import image_tables as g
import slot_owners as s

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

PIN = re.compile(r"// *(?:VTABLE: *HYPERX +(0x[0-9a-fA-F]+)|SLOT TABLE +(0x[0-9a-fA-F]+))")

# Below this a run is a jump table or a pair of handlers, not a vtable. See the note above.
LEAST = 3

# How far past the last pinned table to keep reading.
TAIL_SLOTS = 40


def pins():
    # Walk every subdirectory of src/.
    found = set()

    for directory, unused, names in os.walk(SRC):
        for name in sorted(names):
            if not name.endswith((".cpp", ".h")):
                continue

            path = os.path.join(directory, name)
            text = open(path, encoding="utf-8", errors="replace").read()

            for first, second in PIN.findall(text):
                found.add(int(first or second, 16))

    return sorted(found)


def main():
    image = g.Image(g.referencePath())
    heads = pins()
    covered = set()

    for position, head in enumerate(heads):
        limit = (heads[position + 1] if position + 1 < len(heads)
                 else head + TAIL_SLOTS * 4)
        at = head

        while at < limit:
            word = image.word(at)

            if word is None or not image.isCode(word):
                break

            covered.add(at)
            at += 4

    rdata = [sec for sec in image.sections if sec[0] == ".rdata"]

    if not rdata:
        print("the image has no .rdata")
        return 1

    unused, start, size, unusedToo = rdata[0]
    runs = []
    run = []
    at = start

    while at < start + size - 4:
        word = image.word(at)

        if word is not None and image.isCode(word) and at not in covered:
            if run and at == run[-1] + 4:
                run.append(at)
            else:
                if len(run) >= LEAST:
                    runs.append(run)

                run = [at]
        else:
            if len(run) >= LEAST:
                runs.append(run)

            run = []

        at += 4

    if len(run) >= LEAST:
        runs.append(run)

    known = s.owners()

    for one in runs:
        print("0x%08x  %d consecutive code pointer(s) that no class names" % (one[0], len(one)))

        for address in one[:6]:
            word = image.word(address)
            who = known.get(word)
            print("    0x%08x  %s" % (word, "%s  %s" % who if who else "not in the tree"))

    print("%d pinned table(s) cover %d code pointer(s); %d run(s) of %d or more uncovered"
          % (len(heads), len(covered), len(runs), LEAST))

    if runs:
        print("Each is a vtable with no class. Find the constructor that stores it: that names it.")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
