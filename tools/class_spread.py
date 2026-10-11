# Lists classes whose bodies are spread over more than one .cpp, and whether each split is settled.
#
#   python tools/class_spread.py          # the summary line
#   python tools/class_spread.py --list   # every open satellite and every homeless name
#
# A class's home is the file named after it, or else the .cpp beside the header that declares it
# when that file holds the most of its bodies. Every other file holding a body is a satellite. A
# satellite is settled only when symbols/layout_exceptions.csv lists it. A name with no home is
# reported apart, split into namespaces and classes.
#
# Exits 1 when any satellite is open.
#
# Read only.

import collections
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

sys.path.insert(0, os.path.join(ROOT, "tools"))

MARKED = re.compile(r"^// FUNCTION: HYPERX 0x[0-9a-f]+\r?\n(?://[^\r\n]*\r?\n)*"
                    r"[^\r\n(]*?\b([A-Za-z_]\w*)::(~?\w+)\s*\(", re.M)

SKIP = ("src/jpeg/",)

# A file with its own compile flags cannot take another file's bodies: merging it into a /O2
# file would recompile every body in it under the wrong switch.
FLAGGED = re.compile(r"set_source_files_properties\((.*?)PROPERTIES\s+COMPILE_FLAGS", re.S)


DECLARES = re.compile(r"^\s*(?:class|struct)\s+([A-Z]\w*)\s*(?:final\s*)?(?::[^;{]*)?\{", re.M)

# A family of free functions under a namespace, which the count reports apart from classes.
NAMESPACE = re.compile(r"^\s*namespace\s+([A-Z]\w*)", re.M)


def own_name(shown):
    return os.path.basename(shown)[:-4].lower().replace("_", "")


def namespaces():
    """Every name this tree opens as a namespace, in a header or a source."""
    found = set()

    for directory, unused, names in os.walk("src"):
        for name in sorted(names):
            if not name.endswith((".h", ".cpp")):
                continue

            shown = os.path.join(directory, name).replace(os.sep, "/")

            if shown.startswith(SKIP):
                continue

            found |= set(NAMESPACE.findall(
                open(shown, encoding="utf-8", errors="replace").read()))

    return found


def declaring_headers():
    """The header each class is declared in, for the classes no file is named after."""
    found = {}

    for directory, unused, names in os.walk("src"):
        for name in sorted(names):
            if not name.endswith(".h"):
                continue

            shown = os.path.join(directory, name).replace(os.sep, "/")

            if shown.startswith(SKIP):
                continue

            text = open(shown, encoding="utf-8", errors="replace").read()

            for cls in DECLARES.findall(text):
                found.setdefault(cls, shown)

    return found


def flagged():
    """Every source CMakeLists.txt gives its own COMPILE_FLAGS."""
    text = open("CMakeLists.txt", encoding="utf-8", errors="replace").read()
    found = set()

    for block in FLAGGED.findall(text):
        found.update(re.findall(r"(src/[A-Za-z0-9_/]+\.cpp)", block))

    return found


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    bodies = collections.defaultdict(collections.Counter)
    texts = {}

    for directory, unused, names in os.walk("src"):
        for name in sorted(names):
            if not name.endswith(".cpp"):
                continue

            shown = os.path.join(directory, name).replace(os.sep, "/")

            if shown.startswith(SKIP):
                continue

            text = open(shown, encoding="utf-8", errors="replace").read()
            texts[shown] = text

            for hit in MARKED.finditer(text):
                bodies[hit.group(1)][shown] += 1

    spread = {cls: files for cls, files in bodies.items() if len(files) > 1}
    own_flags = flagged()
    declared = declaring_headers()
    families = namespaces()
    open_ones = []
    unnamed = []
    settled = 0

    for cls in sorted(spread):
        files = spread[cls]
        home = None

        for shown in files:
            if own_name(shown) == cls.lower().replace("_", ""):
                home = shown

        # The header that declares the class names its home when no file is named after it, but
        # only when its .cpp holds the most bodies. A leading C is not stripped: most C<Name>
        # classes have a file of the exact name.
        if home is None and cls in declared:
            beside = declared[cls][:-2] + ".cpp"

            if beside in files and files[beside] == max(files.values()):
                home = beside

        # With no home, the class is reported as unnamed rather than guessing one from the
        # body count.
        if home is None:
            unnamed.append((cls, sorted(files)))
            continue

        for shown, count in sorted(files.items()):
            if shown == home:
                continue

            why = settles(cls, shown, texts[shown], own_flags)

            if why:
                settled += 1
            else:
                open_ones.append((cls, shown, count, home))

    if show:
        for cls, shown, count, home in open_ones:
            print("%-30s %2d bod%s in %-38s home %s"
                  % (cls, count, "y" if count == 1 else "ies", shown, os.path.basename(home)))

    if show:
        for cls, where in unnamed:
            print("%-30s no file is named after this %s: %s"
                  % (cls, "namespace" if cls in families else "class",
                     ", ".join(os.path.basename(one) for one in where)))

    families_unnamed = sum(1 for cls, unused in unnamed if cls in families)

    print("%d class(es) with bodies in more than one file, over %d satellite file(s): %d "
          "settled by symbols/layout_exceptions.csv, "
          "%d open, %d name(s) no file is named after of which %d are namespaces and %d classes"
          % (len(spread), settled + len(open_ones), settled, len(open_ones), len(unnamed),
             families_unnamed, len(unnamed) - families_unnamed))

    return 1 if open_ones else 0


def listed():
    """The satellites symbols/layout_exceptions.csv settles."""
    with open(os.path.join(ROOT, "symbols", "layout_exceptions.csv"), encoding="utf-8") as f:
        rows = [line for line in f if line.strip() and not line.startswith("#")]
    return set(row.split(",", 1)[0] for row in rows[1:])


def settles(cls, shown, text, own_flags):
    """Why this satellite holds a body of `cls`, or "" when nothing says."""
    if shown in LISTED:
        return "listed"

    return ""


LISTED = listed()


if __name__ == "__main__":
    sys.exit(main())
