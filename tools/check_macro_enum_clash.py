# Finds an enumerator that another file also defines as a macro.
#
# Usage: python tools/check_macro_enum_clash.py
# Always exits 0; the count is the result.
#
# The preprocessor rewrites the enumerator's name, so `PLAN_ARG_PRONOUN = 0xfedc,` becomes
# `0xfedc = 0xfedc,` and the enum stops parsing. That happens only once the file gains an include
# that reaches the macro, so a clean build does not prove a new header macro is safe. Pairs in
# the same file are skipped, since they fail the build at once.

import os
import re
import sys

DEFINE = re.compile(r"^#define ([A-Z][A-Z0-9_]*)\s", re.M)
ENUM_BLOCK = re.compile(r"enum\s+\w*\s*\{([^}]*)\}", re.S)
ENUMERATOR = re.compile(r"^\s*([A-Z][A-Z0-9_]*)\s*(?:=|,|$)", re.M)


def sourceNames(root):
    # Every file under root, recursively, as a path relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def header_macros(root, suffix=".h"):
    """Macro name to the file that defines it, over one suffix."""
    macros = {}

    for name in sorted(sourceNames(root)):
        if not name.endswith(suffix):
            continue

        path = os.path.join(root, name)

        for macro in DEFINE.findall(open(path, encoding="utf-8", errors="replace").read()):
            macros.setdefault(macro, name)

    return macros


def main():
    root = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "src")

    macros = header_macros(root)
    # A .cpp macro can clash with a header enum the file includes later.
    for macro, where in header_macros(root, ".cpp").items():
        macros.setdefault(macro, where)

    total = 0

    for name in sorted(sourceNames(root)):
        if not name.endswith((".cpp", ".h")):
            continue

        text = open(os.path.join(root, name), encoding="utf-8", errors="replace").read()

        for block in ENUM_BLOCK.findall(text):
            for enumerator in ENUMERATOR.findall(block):
                if enumerator not in macros or macros[enumerator] == name:
                    continue

                total += 1
                print("src/%s: enumerator %s is a macro in src/%s"
                      % (name, enumerator, macros[enumerator]))

    print("%d enumerator(s) a header also defines as a macro" % total)

    return 0


if __name__ == "__main__":
    sys.exit(main())
