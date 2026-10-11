# List the class each .cpp file defines bodies for.
#
#     python tools/class_files.py            # summary counts
#     python tools/class_files.py --list     # every split file, OPEN or the reason it is held
#     python tools/class_files.py --free     # every file with no qualified body
#
# The target is one class per .h/.cpp pair. Files fall into three groups:
#
#   SPLIT    one cpp with bodies for several classes. A split is held, not open, only when
#            symbols/layout_exceptions.csv lists the file.
#   FREE     one cpp with no qualified body. Some are free functions and some are a class whose
#            bodies are all static or in a namespace; read before moving.
#   PAIRED   one cpp, one class.
#
# Moving a body is not free: its translation unit decides inlining and the link order decides
# where it lands. Move one class, measure, commit. See docs/RE_CRUFT.md.

import collections
import os
import re
import sys

# A definition at column nought whose name carries a class qualifier. The optional return type
# in front may hold spaces, stars and template punctuation; the qualifier is the last Name::
# before the parameter list, and a destructor's tilde is part of the function name.
QUALIFIED = re.compile(r"^(?!\s)(?:[\w:<>,* &]+?[ \*&])?([A-Z]\w+)::~?\w+\s*\(", re.M)

# A qualifier can be a namespace for free functions, which does not make its file a shared one.
# The tree's own namespace declarations say which qualifiers those are.
NAMESPACE = re.compile(r"^\s*namespace\s+([A-Z]\w*)", re.M)
DECLARED = re.compile(r"^\s*(?:class|struct)\s+([A-Z]\w*)\s*(?:final\s*)?(?::[^;{]*)?\{", re.M)


def namespaces(root):
    """Every qualifier the tree opens as a namespace and never declares as a class."""
    opened = set()
    classes = set()

    for directory, unused, names in os.walk(root):
        for name in names:
            if not name.endswith((".cpp", ".h")):
                continue

            text = open(os.path.join(directory, name),
                           encoding="utf-8", errors="replace").read()
            opened |= set(NAMESPACE.findall(text))
            classes |= set(DECLARED.findall(text))

    return opened - classes


def sourceNames(root):
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            if name.endswith(".cpp"):
                found.append(os.path.join(directory, name))

    return sorted(found)


def listed():
    """The files symbols/layout_exceptions.csv settles."""
    path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "symbols",
                        "layout_exceptions.csv")

    with open(path, encoding="utf-8") as f:
        rows = [line for line in f if line.strip() and not line.startswith("#")]

    return set(row.split(",", 1)[0] for row in rows[1:])


LISTED = listed()


def held(text, shown, counts):
    """The reason this file's split is settled, or "" when it is genuinely open."""
    if shown in LISTED:
        return "listed"

    return ""


def collect(root):
    skip = namespaces(root)
    split = []
    free = []
    paired = 0

    for path in sourceNames(root):
        text = open(path, encoding="utf-8", errors="replace").read()
        counts = collections.Counter(n for n in QUALIFIED.findall(text)
                                     if n not in skip)
        shown = os.path.relpath(path, os.path.dirname(root)).replace(os.sep, "/")
        lines = len(text.splitlines())

        if len(counts) > 1:
            split.append((len(counts), lines, shown, counts, held(text, shown, counts)))
        elif not counts:
            free.append((lines, shown))
        else:
            paired += 1

    return sorted(split, reverse=True), sorted(free, reverse=True), paired


def main():
    root = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "src")
    show = "--list" in sys.argv
    wantFree = "--free" in sys.argv

    split, free, paired = collect(root)

    if show:
        for count, lines, path, counts, why in split:
            print("%-7s %2d classes %5d lines  %-44s %s"
                  % (why.upper() or "OPEN", count, lines, path,
                     ", ".join(n for n, unused in counts.most_common(5))))
    if wantFree:
        for lines, path in free:
            print("FREE   %5d lines  %s" % (lines, path))

    openSplit = [row for row in split if not row[4]]

    print("%d cpp file(s) define bodies for more than one class, of which %d are open and "
          "%d are held by symbols/layout_exceptions.csv; %d define none, "
          "%d are one class to one file"
          % (len(split), len(openSplit), len(split) - len(openSplit), len(free), paired))

    return 0


if __name__ == "__main__":
    sys.exit(main())
