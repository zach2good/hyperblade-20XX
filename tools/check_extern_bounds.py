# Report an `extern` array in src whose first dimension has no bound.
#
# `extern int g_aSlots[];` tells no reader how far the storage reaches, and reccmp compares a
# GLOBAL marker on it over no span. Write the bound the original's storage has, even where the
# definition's initializer already sizes it. The measured build is not needed.
#
#   python tools/check_extern_bounds.py
#
# Read only. Exit code 1 when an unbounded extern array stands.

import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# One declaration statement starting with `extern`, up to its semicolon.
EXTERN = re.compile(r"\bextern\b(?!\s*\"C\"\s*\{)([^;{}]*);")
UNBOUNDED = re.compile(r"\b(\w+)\s*\[\s*\]")
DECLARATOR = re.compile(r"\(\s*(?:__cdecl|__stdcall|__fastcall|WINAPI|CALLBACK)?\s*\*")

# The IJG library's own headers, which are linked, not written.
SKIP = ("src/jpeg/",)


def main():
    os.chdir(ROOT)
    faults = []
    seen = 0

    for path in sorted(glob.glob(os.path.join("src", "**", "*.*"), recursive=True)):
        shown = path.replace(os.sep, "/")

        if not shown.endswith((".h", ".cpp", ".c")) or shown.startswith(SKIP):
            continue

        body = code(open(path, encoding="utf-8", errors="replace").read(), strings=True)

        for hit in EXTERN.finditer(body):
            decl = hit.group(1)
            bracket = decl.find("[")

            if bracket < 0:
                continue

            # A parenthesis before the bracket is a function's, unless it opens a declarator
            # such as `(*g_apfn[4])`.
            paren = decl.find("(")

            if 0 <= paren < bracket and not DECLARATOR.match(decl, paren):
                continue

            seen += 1
            unbounded = UNBOUNDED.search(decl)

            if unbounded:
                line = body.count("\n", 0, hit.start()) + 1
                faults.append((shown, line, unbounded.group(1)))

    for shown, line, name in faults:
        print("%s:%d: %s is an extern array with no bound" % (shown, line, name))

    print("%d extern array(s), %d with no bound" % (seen, len(faults)))

    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
