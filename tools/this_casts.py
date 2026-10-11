"""Report every member that casts `this` to a class it has no relation to.

tools/check_self_casts.py fails on the two shapes that are always wrong. This reports the wider
set, because several of its answers are deliberate. A cast of `this` to an unrelated class means
one of these:

  - The two are one object and one of them is a wrapper holding a single member, which can be
    folded into the other.
  - The two are one object and the tree has not decided which name to keep, like `AgendaItem`
    and `InteractorNodeBase`. docs/class_hierarchy.md has the evidence.
  - This tree reaches a shared body with the object already in ECX. That is deliberate.

A cast to a base or derived class is ordinary and not reported, and neither is `(char*)this`,
the tree's spelling for byte-offset access.

    python tools/this_casts.py              # pairs, most sites first
    python tools/this_casts.py --sites      # every site, with its file and method
"""

import os
import re
import sys
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

DEFINITION = re.compile(r"^(?:[\w:*&<>\s]+?)\b([0-9A-Za-z_]+)::([~0-9A-Za-z_]+)\s*\([^;{]*\)"
                        + r"\s*(?:const\s*)?" + chr(10) + r"?\{", re.M)
# A local holding the object counts as `this`, such as `char* const pThis = (char*)this;`.
# `(char*)this + 0x98` is a sub-object's address, not the object, and a member of the holder,
# such as `pSelf->m_pCamera`, is not the holder.
HOLDER = re.compile(r"([0-9A-Za-z_]+)" + chr(92) + r"s*=" + chr(92)
                    + r"s*" + chr(92) + r"(" + chr(92) + r"s*(?:const" + chr(92)
                    + r"s+)?[0-9A-Za-z_]+" + chr(92) + r"s*" + chr(92) + r"*" + chr(92)
                    + r"s*" + chr(92) + r")" + chr(92) + r"s*this(?![0-9A-Za-z_])"
                    + r"(?!" + chr(92) + r"s*[-+[])")

CAST_TEMPLATE = (r"" + chr(92) + r"(" + chr(92) + r"s*(?:const" + chr(92) + r"s+)?"
                 + r"([0-9A-Za-z_]+)" + chr(92) + r"s*" + chr(92) + r"*" + chr(92)
                 + r"s*" + chr(92) + r")" + chr(92) + r"s*(?:" + chr(92) + r"("
                 + chr(92) + r"s*void" + chr(92) + r"s*" + chr(92) + r"*" + chr(92)
                 + r"s*" + chr(92) + r")" + chr(92) + r"s*)?(?:%s)(?![0-9A-Za-z_])"
                 + r"(?!" + chr(92) + r"s*(?:->|\.|\[))")


# A local the body then moves is no longer the object.
MOVED = re.compile(r"([0-9A-Za-z_]+)" + chr(92) + r"s*(?:" + chr(92) + r"+=|-=|"
                   + chr(92) + r"+" + chr(92) + r"+|--)")

def sourceNames(root):
    # Walk subdirectories; names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def castsIn(names):
    return re.compile(CAST_TEMPLATE % "|".join(re.escape(name) for name in sorted(names)))


def holders(body):
    """Every local that holds the object, plus `this` itself."""
    return (set(HOLDER.findall(body)) - set(MOVED.findall(body))) | set(["this"])


DERIVES = re.compile("^class ([0-9A-Za-z_]+) *: *public ([0-9A-Za-z_]+)", re.M)

# Casting `this` to one of these is a reinterpretation, not a claim about the object.
PRIMITIVE = set("char void int short long float double unsigned signed const".split())


def sources(suffixes):
    for name in sorted(sourceNames(SRC)):
        if name.endswith(suffixes):
            text = open(os.path.join(SRC, name), encoding="utf-8", errors="replace").read()

            yield name, text.replace(chr(13) + chr(10), chr(10))


def bases():
    found = {}

    for unused, text in sources((".cpp", ".h")):
        for derived, base in DERIVES.findall(text):
            found[derived] = base

    return found


def ancestry(klass, known):
    """Every class above this one. The guard is on the class about to be visited, so the walk
    reaches every generation."""
    found = set()

    while klass in known:
        klass = known[klass]

        if klass in found:
            break

        found.add(klass)

    return found


def main():
    showSites = "--sites" in sys.argv[1:]
    known = bases()
    rows = []

    for name, text in sources((".cpp",)):
        for match in DEFINITION.finditer(text):
            klass = match.group(1)
            end = text.find(chr(10) + "}" + chr(10), match.end())
            body = text[match.end():end if end > 0 else len(text)]

            for target in sorted(set(castsIn(holders(body)).findall(body))):
                if target in PRIMITIVE or target == klass:
                    continue

                if target in ancestry(klass, known) or klass in ancestry(target, known):
                    continue

                rows.append((name, klass, match.group(2), target))

    counted = Counter((klass, target) for unused, klass, unusedToo, target in rows)

    if showSites:
        for name, klass, method, target in sorted(rows):
            print("%-30s %s::%s casts this to %s" % (name, klass, method, target))
    else:
        for (klass, target), count in sorted(counted.items(),
                                             key=lambda entry: (-entry[1], entry[0])):
            where = sorted(set(name for name, left, unused, right in rows
                               if left == klass and right == target))
            print("%-26s -> %-26s %d  %s" % (klass, target, count, " ".join(where[:3])))

    print("%d site(s) in %d pair(s)" % (len(rows), len(counted)))

    return 0


if __name__ == "__main__":
    sys.exit(main())
