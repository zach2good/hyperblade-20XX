# Report a `Name__Function` in src: a class or namespace spelled into a free function's name.
#
# Every function is a member or lives in a named namespace, so `Name__` belongs as `Name::`.
# This reports each such name that code declares, defines or calls. Comments, string and
# character literals are skipped, and so is src/jpeg/markers.h, whose markers name the linked
# library's routines.
#
#   python tools/check_prefixed_functions.py
#
# Read only. Exit code 1 when any is found.

import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SKIP = ("src/jpeg/markers.h",)

PREFIXED = re.compile(r"\b([A-Z][A-Za-z0-9]*__[A-Za-z0-9_]+)\s*\(")
LEXEME = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.S)


def blank(match):
    """Keep a comment's or literal's line breaks so line numbers survive."""
    return re.sub(r"[^\n]", " ", match.group(0))


def main():
    os.chdir(ROOT)
    faults = []

    for suffix in ("*.cpp", "*.h", "*.c"):
        for path in sorted(glob.glob(os.path.join("src", "**", suffix), recursive=True)):
            shown = path.replace(os.sep, "/")

            if shown in SKIP:
                continue

            text = open(shown, encoding="latin-1", newline="").read().replace("\r\n", "\n")
            code = LEXEME.sub(blank, text)

            for match in PREFIXED.finditer(code):
                line = code.count("\n", 0, match.start()) + 1
                faults.append((shown, line, match.group(1)))

    for shown, line, name in faults:
        print("%s:%d %s is a prefixed free function; make it a member or put it in a namespace"
              % (shown, line, name))

    print("%d prefixed function name(s) in src" % len(faults))

    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
