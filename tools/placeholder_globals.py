# Lists globals whose name gives only their address, with the functions that read and write each.
#
#   python tools/placeholder_globals.py            # the summary line
#   python tools/placeholder_globals.py --list     # one row per global, with its readers and writers
#   python tools/placeholder_globals.py --max N    # also fail when there are more than N
#
# A name is a placeholder when it embeds an address in the image, as g_flInit004B14A0 or
# g_wPoolCount00004BB444 did, or when it is a Ghidra default such as DAT_004b14a0. Only a
# declaration or definition at file scope counts; a name that only a comment mentions does not.
#
# A use is a write when the name is the target of an assignment or an increment, and a read
# otherwise. The function a use sits in is the one declared on the first code line after the
# nearest marker above it.
#
# Read only.

import collections
import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

IMAGE_LOW = 0x400000
IMAGE_HIGH = 0x500000

NAME = re.compile(r"(?:[gs]_\w*?|(?:DAT|PTR)_)([0-9A-Fa-f]{6,})")
WORD = re.compile(r"\b(?:[gs]|DAT|PTR)_\w+")

# A declaration or definition at column nought: storage words, a type, then the name.
DECL = re.compile(r"^(?:(?:extern|static|const|unsigned|signed|struct|class)\s+|\"C\"\s+)*"
                  r"[\w:]+[\s*&]+(?:const\s+)?(\w+)\s*(?:\[[^\]\n]*\]\s*)*[;=\[(]", re.M)

MARKER = re.compile(r"//\s*(?:FUNCTION|STUB|LIBRARY|SYNTHETIC):\s*HYPERX\s+0x[0-9a-fA-F]+")
SIGNATURE = re.compile(r"([\w:~]+)\s*\(")

# What follows a name that is being written: an assignment, compound or not, or an increment.
WRITTEN = re.compile(r"\w+(?:\.\w+|\[[^\]\n]*\])*\s*(?:(?:[-+*/|&^]|<<|>>)?=(?!=)|\+\+|--)")


def placeholder(name):
    hit = NAME.fullmatch(name)

    if hit is None:
        return False

    # The run can start with a hex letter from the word before it, as Background4C6484 does.
    run = hit.group(1)

    return any(IMAGE_LOW <= int(run[-width:], 16) < IMAGE_HIGH
               for width in (8, 7, 6) if len(run) >= width)


def written(before, after):
    """Whether the use of the name that starts `after` is a write."""
    before = before.rstrip()

    return before.endswith("++") or before.endswith("--") or WRITTEN.match(after) is not None


def owners(raw):
    """For each line, the function whose marker is nearest above it."""
    result = []
    here = "(file scope)"
    waiting = False

    for line in raw.split("\n"):
        stripped = line.strip()

        if MARKER.search(line):
            waiting = True
        elif waiting and stripped and not stripped.startswith("//") \
                and not stripped.startswith("#"):
            found = SIGNATURE.search(stripped)
            here = found.group(1) if found else stripped[:40]
            waiting = False

        result.append(here)

    return result


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv
    ceiling = None

    if "--max" in sys.argv:
        ceiling = int(sys.argv[sys.argv.index("--max") + 1])

    paths = sorted(glob.glob(os.path.join("src", "**", "*.h"), recursive=True)
                   + glob.glob(os.path.join("src", "**", "*.cpp"), recursive=True))

    texts = {}
    declared = collections.defaultdict(set)

    for path in paths:
        shown = os.path.relpath(path, "src").replace(os.sep, "/")
        raw = open(path, encoding="utf-8", errors="replace").read().replace("\r\n", "\n")
        text = code(raw, strings=True)
        texts[shown] = (raw, text)

        for hit in DECL.finditer(text):
            if placeholder(hit.group(1)):
                declared[hit.group(1)].add(shown)

    reads = collections.defaultdict(collections.Counter)
    writes = collections.defaultdict(collections.Counter)

    for shown, (raw, text) in texts.items():
        if not any(name in text for name in declared):
            continue

        lines = text.split("\n")
        where = owners(raw)

        for number, line in enumerate(lines):
            # A declaration, at file scope or inside a function, is neither a read nor a write.
            decl = DECL.match(line.lstrip())

            for hit in WORD.finditer(line):
                name = hit.group(0)

                if name not in declared:
                    continue

                if decl is not None and decl.group(1) == name:
                    continue

                site = "%s %s" % (shown, where[number])

                if written(line[:hit.start()], line[hit.start():]):
                    writes[name][site] += 1
                else:
                    reads[name][site] += 1

    names = sorted(declared, key=lambda name: (-sum(reads[name].values()), name))
    unread = [name for name in names if not reads[name]]

    if show:
        for name in names:
            print("%-32s %3d read(s) %3d write(s)  %s"
                  % (name, sum(reads[name].values()), sum(writes[name].values()),
                     " ".join(sorted(declared[name]))))

            for site in sorted(reads[name]):
                print("    reads  %s" % site)

            for site in sorted(writes[name]):
                print("    writes %s" % site)

    print("%d global(s) named for their address: %d read somewhere, %d never read"
          % (len(names), len(names) - len(unread), len(unread)))

    if ceiling is not None and len(names) > ceiling:
        print("more than the ceiling of %d; name the new one for what it holds" % ceiling)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
