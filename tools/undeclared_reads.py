"""Rank classes by the bytes their own members reach and the class never declares.

Each is a body reaching its object as `*(int*)(pThis + 0x2c)` because the class has no name
for that word. The classes at the top are where a decode buys the most.

Every class tiles with no hole, so the field above an offset covers it, and a read inside a
named field (an array element, say) is declared. Only a read that lands in a reserve, or past
the last field of a class with no client-stated size, counts. A base's fields count as the
derived class's.

    python tools/undeclared_reads.py              # classes, most undeclared offsets first
    python tools/undeclared_reads.py Radar        # the offsets and the members that read them
"""
import os
import re
import sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import field_names as fn

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

DEFINITION = re.compile(r"^(?:[\w:*&<>\s]+?)\b([0-9A-Za-z_]+)::([~0-9A-Za-z_]+)\s*\([^;{]*\)"
                        + r"\s*(?:const\s*)?" + chr(10) + r"?\{", re.M)

MACRO = re.compile(r"^#define ([A-Z][A-Z0-9_]*) (0x[0-9a-fA-F]+)$", re.M)

# A reserve says how many bytes are undescribed, not what is there, so a read inside one counts.
GAP = re.compile(r"^(?:m_)?(?:a)?(?:gap|reserved|undecoded|head|body|storage|block|pad)", re.I)

# Every field's own offset, gap or not, so a read can be placed between two of them.
ANY_FIELD = re.compile(r"^[ 	]+(?!virtual|typedef)"
                       + r"(?:(?:(?!p(?:ublic|rotected|rivate)\s*:)[\w:*&<>])[\w:*&<>\s]*?)"
                       + r"[\s*]([0-9A-Za-z_]+)(?:\[[^;]*\])?;[ 	]*//\s*(0x[0-9a-fA-F]+)",
                       re.M)


PROBE = re.compile(r"typedef char Probe_([0-9A-Za-z_]+)\[sizeof\([0-9A-Za-z_]+\) *== *"
                   + r"(0x[0-9a-fA-F]+|\d+) *\?")


def sizes():
    """{class: its size, where the client stated one}."""
    found = {}

    for unused, text in fn.sources((".h", ".cpp")):
        for klass, value in PROBE.findall(text):
            found[klass] = int(value, 16) if value.startswith("0x") else int(value)

    return found


def described():
    """{class: [(offset, whether the field there is a reserve)]}, its own fields and its bases'.

    tools/check_field_tiling.py enforces that classes tile with no hole, so the field above an
    offset is the field that covers it.
    """
    own = {}
    base = {}

    for unused, text in fn.sources((".h", ".cpp")):
        for match in fn.CLASS.finditer(text):
            if match.group(1) in own:
                continue

            base[match.group(1)] = match.group(2)
            own[match.group(1)] = sorted((int(offset, 16), bool(GAP.match(name)))
                                         for name, offset in ANY_FIELD.findall(match.group(3)))

    found = {}

    for klass in own:
        rows = []
        at = klass
        seen = set()

        while at in own and at not in seen:
            seen.add(at)
            rows.extend(own[at])
            at = base.get(at)

        found[klass] = sorted(rows)

    return found


def reads():
    """{class: {offset: set of member names that reach it}}."""
    table = fn.fields()
    covered = described()
    limits = sizes()
    found = defaultdict(lambda: defaultdict(set))

    for unused, text in fn.sources((".cpp",)):
        macros = dict((name, int(value, 16)) for name, value in MACRO.findall(text))

        for hit in DEFINITION.finditer(text):
            klass = hit.group(1)
            end = text.find(chr(10) + "}" + chr(10), hit.end())
            body = text[hit.end():end if end > 0 else len(text)]
            known = table.get(klass, {})

            for who in fn.holders(body):
                pattern = re.compile(r"(?:" + re.escape(who) + r")\s*\+\s*"
                                     + r"(0x[0-9a-fA-F]+|[A-Z][A-Z0-9_]{2,})")

                for word in pattern.findall(body):
                    offset = int(word, 16) if word.startswith("0x") else macros.get(word)

                    if offset is None or offset in known:
                        continue

                    rows = covered.get(klass, ())
                    above = [row for row in rows if row[0] <= offset]

                    if not above or above[-1][1]:
                        found[klass][offset].add(hit.group(2))
                        continue

                    # Inside a named field. Past the last field, only a size the client states
                    # can place the read inside the object; without one the read counts.
                    if above[-1][0] != rows[-1][0]:
                        continue

                    if offset < limits.get(klass, 0):
                        continue

                    found[klass][offset].add(hit.group(2))

                    found[klass][offset].add(hit.group(2))

    return found


def main():
    found = reads()
    wanted = [a for a in sys.argv[1:] if not a.startswith("-")]

    if wanted:
        for klass in wanted:
            print("%s reaches %d offset(s) it does not declare:" % (klass, len(found[klass])))

            for offset in sorted(found[klass]):
                print("    0x%-6x %s" % (offset, " ".join(sorted(found[klass][offset]))))

        return 0

    print("%d class(es) reach bytes they do not declare:" % len(found))

    for klass in sorted(found, key=lambda k: (-len(found[k]), k))[:30]:
        print("    %-32s %3d  %s"
              % (klass, len(found[klass]),
                 " ".join("0x%x" % o for o in sorted(found[klass])[:10])))

    return 0


if __name__ == "__main__":
    sys.exit(main())
