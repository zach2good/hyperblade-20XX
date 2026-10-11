# Find records whose first member is a byte run, which is a view rather than a decode.
#
# `char undecoded00[0xf24];` in front of one array pads the way to the one member the struct
# exists for, and the struct exists because some pointer has no type. Type the pointer and the
# view goes away. A genuine undecoded run sits between named fields, so a run that opens a record
# and is longer than everything after it is the shape reported.
#
#     python tools/undecoded_views.py           # totals only
#     python tools/undecoded_views.py --list    # every record
#
# Read only. What the view is a view of comes from reading the cast sites.

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

BLOCK = re.compile(r"\b(struct|class)\s+(\w+)\s*(?::[^;{]*)?\{(.*?)\n\};", re.S)
RUN = re.compile(r"^\s*char\s+(\w*(?:undecoded|gap)\w*)\s*\[([^\]]+)\]\s*;")
SKIP = ("public:", "private:", "protected:", "//", "/*", "*")


def sources(root):
    found = []

    for directory, unused, names in os.walk(root):
        for name in sorted(names):
            if name.endswith(".h"):
                found.append(os.path.join(directory, name))

    return found


def size(text):
    """The declared extent of a run, which may be an expression like 0x100 - 0xf4."""
    try:
        return eval(text, {"__builtins__": {}}, {})
    except Exception:
        return 0


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    found = []

    for path in sources("src"):
        shown = os.path.relpath(path, "src").replace(os.sep, "/")
        text = open(path, encoding="utf-8", errors="replace").read()

        for kind, name, body in BLOCK.findall(text):
            lines = [line for line in body.split("\n")
                     if line.strip() and not line.strip().startswith(SKIP)]

            if not lines:
                continue

            hit = RUN.match(lines[0])

            if not hit:
                continue

            lead = size(hit.group(2))
            # The rest of the record, counted as declarations rather than bytes: a view has
            # one or two members behind its run and a real record has many.
            rest = len([line for line in lines[1:] if ";" in line])

            if lead >= 0x20 and rest <= 4:
                found.append((shown, kind, name, lead, rest))

    if show:
        for shown, kind, name, lead, rest in found:
            print("VIEW   %-34s %-6s %-24s 0x%x of padding, %d member(s) behind it"
                  % (shown, kind, name, lead, rest))

    print("%d record(s) open with a byte run longer than 0x20 and hold four members or fewer"
          % len(found))

    return 0


if __name__ == "__main__":
    sys.exit(main())
