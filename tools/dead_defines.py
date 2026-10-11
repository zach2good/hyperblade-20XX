"""Find, and optionally delete, offset defines that nothing uses.

A define is dead when its name appears exactly once in the text searched: its own definition.
Uses inside other macros, `#ifdef HYPERX_BRINGUP` blocks and comments all count, so a name a
comment still discusses is left alone.

A define in a .cpp is searched in its own file. A define in a header is searched across the
whole tree, since every includer can use it. That can miss a dead define whose name another
file reuses, which is the safe direction.

Only `#define NAME <number>` is considered. Function-like macros, expressions and continuations
are skipped.

    python tools/dead_defines.py            report
    python tools/dead_defines.py --delete   remove them

Deleting should not move a byte, but measure anyway: run tools/prove_neutral.py --save before
and the bare comparison after. A report exits 1 when any dead define is found.
"""

import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# A plain numeric define and nothing else. The trailing `$` refuses a line continuation.
DEFINE = re.compile(r"^#define ([A-Z][A-Z0-9_]+) +(?:0x[0-9a-fA-F]+|\d+)[ \t]*$", re.M)


def dead_in(text, corpus=None):
    """Every plain numeric define in `text` whose name appears nowhere but its definition.

    `corpus` is what each name is counted in when it is not the text itself, which is what a
    header needs: one occurrence there is the definition and anything more is a use.
    """
    found = []
    where = text if corpus is None else corpus

    for hit in DEFINE.finditer(text):
        name = hit.group(1)

        if len(re.findall(r"\b" + re.escape(name) + r"\b", where)) == 1:
            found.append((name, hit.start(), hit.end()))

    return found


def main():
    delete = "--delete" in sys.argv[1:]
    total = 0
    touched = 0

    paths = sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True)
                   + glob.glob(os.path.join(ROOT, "src", "**", "*.h"), recursive=True))
    whole = "".join(open(p, encoding="utf-8").read() for p in paths)

    for path in paths:
        text = open(path, encoding="utf-8").read()

        # A header's names belong to everyone who includes it.
        if path.endswith(".h"):
            found = dead_in(text, whole)
        else:
            found = dead_in(text)

        if not found:
            continue

        total = total + len(found)
        touched = touched + 1
        print("%-34s %2d  %s" % (os.path.basename(path), len(found),
                                 " ".join(n for n, unused_a, unused_b in found[:6])))

        if not delete:
            continue

        # Backwards, so an earlier span's offsets are still right after a later cut.
        for unused, start, end in reversed(found):
            after = end

            if text[after:after + 1] == chr(10):
                after = after + 1

            text = text[:start] + text[after:]

        open(path, "w", encoding="utf-8", newline="").write(text)

    print()
    print("%d dead define(s) across %d file(s)%s"
          % (total, touched, ", deleted" if delete else ""))

    # A report fails and a deletion does not. tools/check_dead_defines.py covers only .cpp files;
    # this covers headers too.
    return 0 if delete else (1 if total else 0)


if __name__ == "__main__":
    sys.exit(main())
