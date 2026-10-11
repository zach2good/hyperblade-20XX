"""Fail when a class's declared field offsets go backwards.

Every field carries its offset as a comment, and the declarations are in order, so the offsets
climb. When they stop climbing, the class declares the same bytes twice: a second run of names
starting over at a low offset. VC6 lays that run out after the first, so its offset comments are
wrong and every reader that reaches it by name reads the wrong word.

    python tools/check_field_offsets.py
    python tools/check_field_offsets.py --all     list every class it read

A gap comment (`char undecodedNN[...]`) counts like any other field: it carries an offset and it
has to climb with the rest.

Exit code 1 when a run goes backwards.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

CLASS = re.compile(r"^(?:class|struct) (\w+)(?:\s*:\s*public\s+\w+)?\n\{(.*?)^\};", re.S | re.M)
FIELD = re.compile(r"^\s+[\w:*&<>\s]+?[\s*](\w+)(?:\[[^\]]*\])?;\s*//\s*(0x[0-9a-fA-F]+)", re.M)

# A union member legitimately repeats an offset, and these classes carry one.
ALLOW_EQUAL = True


def sourceNames(root):
    # Paths come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def main():
    show_all = "--all" in sys.argv[1:]
    faults = []
    read = 0

    for name in sorted(sourceNames(SRC)):
        if not name.endswith((".cpp", ".h")):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8",
                       errors="replace").read().replace("\r\n", "\n")

        for match in CLASS.finditer(text):
            offsets = [(field, int(off, 16)) for field, off in FIELD.findall(match.group(2))]

            if len(offsets) < 2:
                continue

            read += 1
            previous = -1
            previous_name = ""

            for field, offset in offsets:
                if offset < previous:
                    faults.append((name, match.group(1), previous_name, previous, field, offset))
                previous, previous_name = offset, field

            if show_all:
                print("  %-26s %-26s %2d fields, 0x%x..0x%x"
                      % (name, match.group(1), len(offsets), offsets[0][1], offsets[-1][1]))

    for where, klass, before, at, field, offset in faults:
        print("%s  %s declares %s at 0x%x and then %s at 0x%x"
              % (where, klass, before, at, field, offset))

    print("%d class(es) read, %d with an offset that goes backwards" % (read, len(faults)))

    if faults:
        print("A run that restarts is the same object declared twice. The second run's offsets")
        print("are what the comments say and not where the compiler puts them.")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
