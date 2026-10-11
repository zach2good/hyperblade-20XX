# Reports which header defines each class, and whether that header is named after it.
#
# Usage: python tools/header_homes.py [--list]
#   --list  print each class that is not in a header of its own name.
# Read only. Exits 1 when any class is outside a header of its own name.
#
# The header side of tools/class_files.py and tools/namespace_spread.py. A class with marked
# bodies is counted, and so is one with none that is still a class rather than a record: it has a
# virtual function, or MEMBERS or more member functions. A class may keep its own small records in
# its header. A class in a family header, one defining FAMILY or more classes, is counted apart
# from one in a smaller shared header; both are faults. Give each its own header, or rename a
# header that defines only that class.

import collections
import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

DEFINES = re.compile(r"^\s*(?:class|struct)\s+([A-Z]\w*)\s*(?:final\s*)?(?::[^;{]*)?\{", re.M)
MARKED = re.compile(r"^// FUNCTION: HYPERX 0x[0-9a-f]+\r?\n(?://[^\r\n]*\r?\n)*"
                    r"[^\r\n(]*?\b([A-Za-z_]\w*)::(~?\w+)\s*\(", re.M)

SKIP = ("src/jpeg/",)

# Where a header stops being a class and its records and starts being a family.
FAMILY = 7

# How many member functions make a type with no marked body a class rather than a record.
MEMBERS = 3

HEAD = re.compile(r"^\s*(?:class|struct)\s+([A-Z]\w*)\s*(?:final\s*)?(?::[^;{]*)?\{", re.M)
# A member function's name and open parenthesis; a function pointer's `(*` or `(__stdcall*` is not.
MEMBER = re.compile(r"[~\w]+\s*\((?!\s*\*|\s*__\w+\s*\*)")


def unmarked_classes(text):
    """The types a header defines that are classes by shape: a virtual function, or MEMBERS or
    more member functions."""
    found = []

    for hit in HEAD.finditer(text):
        at = hit.end()
        depth = 1

        while depth and at < len(text):
            if text[at] == "{":
                depth += 1
            elif text[at] == "}":
                depth -= 1

            at += 1

        body = text[hit.end():at - 1]
        members = [one for one in MEMBER.findall(body)
                   if not one.startswith(("sizeof", "offsetof", "STATIC_ASSERT"))]

        if "virtual" in body or len(members) >= MEMBERS:
            found.append(hit.group(1))

    return found


def sources(suffix):
    for path in sorted(glob.glob(os.path.join("src", "**", suffix), recursive=True)):
        shown = path.replace(os.sep, "/")

        if not shown.startswith(SKIP):
            yield shown


def own_name(shown):
    return os.path.basename(shown)[:-2].lower().replace("_", "")


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    withbodies = set()

    for shown in sources("*.cpp"):
        for cls, unused in MARKED.findall(open(shown, encoding="utf-8",
                                                  errors="replace").read()):
            withbodies.add(cls)

    where = {}
    held = collections.Counter()
    shaped = set()

    for shown in sources("*.h"):
        text = code(open(shown, encoding="utf-8", errors="replace").read())

        for cls in DEFINES.findall(text):
            where.setdefault(cls, shown)
            held[shown] += 1

        shaped.update(unmarked_classes(text))

    unmarked = sorted(shaped - withbodies)

    named = 0
    family = 0
    other = []

    for cls in sorted(withbodies):
        shown = where.get(cls)

        if shown is None:
            continue

        if own_name(shown) == cls.lower().replace("_", ""):
            named += 1
            continue

        if held[shown] >= FAMILY:
            family += 1
            other.append((cls, shown, held[shown]))
            continue

        other.append((cls, shown, held[shown]))

    loose = []

    for cls in unmarked:
        shown = where.get(cls)

        if shown is not None and own_name(shown) != cls.lower().replace("_", ""):
            loose.append((cls, shown, held[shown]))

    if show:
        for cls, shown, n in other:
            print("%-30s %-40s %d class(es) in that header" % (cls, shown, n))

        for cls, shown, n in loose:
            print("%-30s %-40s %d class(es) in that header, no marked body" % (cls, shown, n))

    print("%d class(es) with marked bodies have a header that defines them: %d in a header of "
          "their own name, %d in a family header of %d or more, %d in neither; %d without marked "
          "bodies, %d of them outside a header of their own name"
          % (named + len(other), named, family, FAMILY, len(other) - family, len(unmarked),
             len(loose)))

    return 1 if other or loose else 0


if __name__ == "__main__":
    sys.exit(main())
