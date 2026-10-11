"""Find bytes that several classes over one base each reserve for themselves.

When two or more classes derived from one base reserve the same offset and length (a
`char m_gapNNN[...]` array) and each then names a field after it, the bytes probably belong to
the base, and every sibling's `// 0xNN` offset comments are wrong. The per-class layout checkers
cannot see this, because each sibling is consistent on its own.

Only reserves count: siblings naming a real field at one offset is a per-kind union. Only a
reserve followed by a field counts: a reserve at the end of a class only pads it to size.

A row is a question, not proof. The constructor decides what a class owns: if moving the
fields onto the base makes its constructors do work the client's do not, the siblings share an
intermediate class instead. Record such an answer in symbols/shared_reserves.csv and the row is
no longer reported.

    python tools/shared_reserves.py

Exit code 1 when a run is reserved by more than one sibling.
"""
import os
import re
import sys
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

CLASS = re.compile(r"^(?:class|struct) ([0-9A-Za-z_]+)(?:\s*:\s*public ([0-9A-Za-z_]+))?\s*"
                   + chr(10) + r"\s*\{(.*?)" + chr(10) + r"\};", re.S | re.M)

# A reserve and its length: the tree spells every one of them as a char array.
RESERVE = re.compile(r"^[ \t]+char ((?:m_)?(?:a)?(?:gap|reserved|undecoded|head|body|storage"
                     + r"|block|pad)[0-9A-Za-z_]*)\[([^\]]+)\]\s*;[ \t]*(?://\s*(0x[0-9a-fA-F]+))?",
                     re.M)

OFFSET_IN_NAME = re.compile(r"([0-9a-fA-F]{2,})$")


def sourceNames(root):
    # Sources live in subdirectories, so walk rather than list. Names are relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def sources():
    for name in sorted(sourceNames(SRC)):
        if name.endswith((".h", ".cpp")):
            text = open(os.path.join(SRC, name), encoding="utf-8", errors="replace").read()

            yield name, text.replace(chr(13) + chr(10), chr(10))


def length(text):
    """The declared length, for the two shapes this tree writes: a literal and a subtraction."""
    text = text.strip()

    if re.fullmatch(r"0x[0-9a-fA-F]+|\d+", text):
        return int(text, 0)

    hit = re.fullmatch(r"(0x[0-9a-fA-F]+)\s*-\s*(0x[0-9a-fA-F]+)", text)

    if hit:
        return int(hit.group(1), 16) - int(hit.group(2), 16)

    return None


def reserves():
    """{(base, offset, length): [class names]} over every reserve that names its offset."""
    found = defaultdict(list)

    for name, text in sources():
        for match in CLASS.finditer(text):
            base = match.group(2)

            if not base:
                continue

            for field, span, comment in RESERVE.findall(match.group(3)):
                size = length(span)

                if size is None:
                    continue

                if comment:
                    at = int(comment, 16)
                else:
                    # Without a comment, the offset is in the name: `char m_gap448[0x1c];`.
                    hit = OFFSET_IN_NAME.search(field)

                    if not hit:
                        continue

                    at = int(hit.group(1), 16)

                # Only a reserve followed by a field counts; that field's offset is what is wrong.
                tail = match.group(3)[match.group(3).index("[" + span + "]"):]

                if not re.search(r";[ 	]*(?://[^" + chr(10) + r"]*)?" + chr(10)
                                 + r"[ 	]*(?!\})[\w:*&<>]", tail):
                    continue

                found[(base, at, size)].append(match.group(1))

    return found


REGISTER = os.path.join(ROOT, "symbols", "shared_reserves.csv")


def accepted():
    """{(base, offset, size)} the register has already answered."""
    found = set()

    for line in open(REGISTER, encoding="utf-8").read().splitlines():
        if line.startswith("#") or line.startswith("base,"):
            continue

        parts = line.split(",", 3)

        if len(parts) >= 3:
            found.add((parts[0], int(parts[1], 16), int(parts[2], 16)))

    return found


def main():
    allowed = accepted()
    rows = []

    for key in sorted(reserves()):
        who = sorted(set(reserves()[key]))

        if len(who) > 1 and key not in allowed:
            rows.append((key, who))

    rows.sort(key=lambda row: (-len(row[1]), row[0]))

    print("%d run(s) are reserved by more than one class over the same base, %d registered:"
          % (len(rows), len(allowed)))

    for (base, at, size), who in rows:
        print("")
        print("    0x%03x for 0x%x, on %d class(es) deriving from %s" % (at, size, len(who), base))
        print("        %s" % " ".join(who))

    if rows:
        print("")
        print("Siblings agreeing on a reserve is the base's layout stated twice. Put the bytes on")
        print("the base, delete every copy, and give the base's first derived field a")
        print("HYPERBLADE_FIELD_AT so the next move stops the build. If the base's constructor")
        print("refuses them, give it a row in symbols/shared_reserves.csv saying so.")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
