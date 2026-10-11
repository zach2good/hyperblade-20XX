# Fails on a cast applied to a name that is then compared straight against zero.
#
#     python tools/check_null_test_casts.py
#     python tools/check_null_test_casts.py --list
#
# In `if ((char*)g_pRenderer != 0)` every pointer type compares against a null pointer constant
# the same way, so the cast decides nothing and emits nothing. Removing one is byte neutral.
#
# Left out on purpose:
#
#   A cast of an address, `(float*)&x != 0`; the pattern asks for a plain name.
#   A cast of a call or an expression, `(T*)f() != 0` or `(T*)(a + b) != 0`, which may convert
#   a value that really is a different type. Only a name, a member chain or a subscript counts.
#   A comparison against anything but zero, `(char*)p != q`, where the cast may be what makes
#   the two comparable.
#
# Gates at zero.

import collections
import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SITE = re.compile(r"\(\s*(?:const\s+)?(?:class\s+|struct\s+)?(\w+)\s*\*+\s*\)\s*"
                  r"([\w>.\-\[\]]+)\s*(?:==|!=)\s*0\b")


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    hits = []

    for path in sorted(glob.glob(os.path.join("src", "**", "*.cpp"), recursive=True)
                       + glob.glob(os.path.join("src", "**", "*.h"), recursive=True)):
        shown = os.path.relpath(path, "src").replace(os.sep, "/")
        text = code(open(path, encoding="utf-8", errors="replace").read())

        for hit in SITE.finditer(text):
            hits.append((shown, text.count("\n", 0, hit.start()) + 1, hit.group(1),
                         hit.group(2)))

    if show:
        for shown, at, kind, name in hits:
            print("%-44s:%-6d (%s*)%s, compared against 0" % (shown, at, kind, name))

    kinds = collections.Counter(kind for unused, unused2, kind, unused3 in hits)

    print("%d cast(s) of a name compared straight against nought%s"
          % (len(hits), ": " + ", ".join("%s*(%d)" % (k, v) for k, v in kinds.most_common(4))
             if hits else ""))

    return 1 if hits else 0


if __name__ == "__main__":
    sys.exit(main())
