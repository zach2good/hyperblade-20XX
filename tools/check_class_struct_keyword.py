# Find types spelled `class` in one file and `struct` in another.
#
# VC6 warns C4099 and carries on, and a standing warning hides new ones, such as C4150 (deleting
# an incomplete type). On a forward declaration the keyword emits nothing, so each fix is free.
# The definition's keyword wins, and the declarations take it.
#
# Gates at zero.

import collections
import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

ANY = re.compile(r"\b(class|struct)\s+([A-Z]\w*)\b")
# A definition rather than a mention: the keyword, the name, an optional base list, a brace.
DEFN = re.compile(r"^\s*(class|struct)\s+(?:__declspec\(\w+\)\s+)?([A-Z]\w*)"
                  r"\s*(?::[^;{]*)?\{", re.M)


def main():
    os.chdir(ROOT)

    paths = sorted(glob.glob(os.path.join("src", "**", "*.h"), recursive=True)
                   + glob.glob(os.path.join("src", "**", "*.cpp"), recursive=True))
    kinds = collections.defaultdict(set)
    where = collections.defaultdict(set)
    defined = {}

    for path in paths:
        shown = os.path.relpath(path, "src").replace(os.sep, "/")
        text = code(open(path, encoding="utf-8", errors="replace").read())

        for kind, name in ANY.findall(text):
            kinds[name].add(kind)
            where[name].add("%s(%s)" % (shown, kind))

        for kind, name in DEFN.findall(text):
            defined.setdefault(name, kind)

    bad = sorted(name for name in kinds if len(kinds[name]) > 1)

    for name in bad:
        print("%-28s defined as %s, mentioned as both: %s"
              % (name, defined.get(name, "?"), ", ".join(sorted(where[name]))[:100]))

    print("%d type(s) spelled class in one place and struct in another" % len(bad))

    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
