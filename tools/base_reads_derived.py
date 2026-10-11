"""Fail when a base's own member reads an offset one of its derived classes declares.

A base cannot read its derived class's layout. The usual shape is a base with a short field list,
a derived class whose first reserve starts too low, and the base's own bodies reaching into it.

tools/shared_reserves.py catches this only when two siblings agree, and tools/check_body_reach.py
only for classes whose length the client states. This uses the derived class's first declaration
as the bound.

What is reported is a base member reading at or past the lowest offset any class standing on
that base declares. The fix is to move the bytes down onto the base and shrink every derived
reserve that covered them.

    python tools/base_reads_derived.py

Exit code 1 when a base reads into its derived class.
"""
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import field_names as fn
import undeclared_reads as ur

REGISTER = os.path.join(ROOT, "symbols", "base_reads.csv")


def firstDeclared():
    """{base: (lowest offset a derived class declares, which class declares it)}."""
    found = {}

    for unused, text in fn.sources((".h", ".cpp")):
        for match in fn.CLASS.finditer(text):
            base = match.group(2)

            if not base:
                continue

            offsets = [int(offset, 16)
                       for unusedName, offset in ur.ANY_FIELD.findall(match.group(3))]

            if not offsets:
                continue

            at = min(offsets)

            if base not in found or at < found[base][0]:
                found[base] = (at, match.group(1))

    return found


def accepted():
    found = set()

    if not os.path.exists(REGISTER):
        return found

    for line in open(REGISTER, encoding="utf-8").read().splitlines():
        if line.startswith("#") or line.startswith("class,"):
            continue

        parts = line.split(",", 3)

        if len(parts) >= 3:
            found.add((parts[0], parts[1], int(parts[2], 16)))

    return found


def main():
    bound = firstDeclared()
    allowed = accepted()
    reads = ur.reads()
    rows = []

    for klass in sorted(reads):
        if klass not in bound:
            continue

        limit, who = bound[klass]

        for offset in sorted(reads[klass]):
            if offset < limit:
                continue

            for method in sorted(reads[klass][offset]):
                if (klass, method, offset) in allowed:
                    continue

                rows.append((klass, method, offset, limit, who))

    for klass, method, offset, limit, who in rows:
        print("%s::%s reads 0x%x, which %s declares from 0x%x"
              % (klass, method, offset, who, limit))

    print("%d base(s) have a derived class; %d read(s) reach into one, %d registered"
          % (len(bound), len(rows), len(allowed)))

    if rows:
        print("A base cannot read its derived class's layout. Move the bytes down onto the base")
        print("and shrink every derived reserve that covered them, or give the read a row in")
        print("symbols/base_reads.csv saying what blocks that.")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
