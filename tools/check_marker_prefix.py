"""Catch a marker whose comment prefix reccmp will not parse.

reccmp wants exactly `// KEYWORD:`. A line like `//// STUB:` builds cleanly and the function
silently vanishes from the report. Exits 1 on any such line.

    python tools/check_marker_prefix.py
"""

import os
import re
import sys

KEYWORDS = ("FUNCTION", "STUB", "GLOBAL", "VTABLE", "STRING", "LIBRARY", "SIZE", "SYNTHETIC")
SUSPECT = re.compile(r"^\s*(/{3,}|/\*+|#+)\s*(" + "|".join(KEYWORDS) + r"):")


def sources():
    for root, _, names in os.walk("src"):
        for name in names:
            if name.endswith((".cpp", ".h")):
                yield os.path.join(root, name)


def main():
    bad = []
    for path in sources():
        for number, line in enumerate(open(path, encoding="utf-8", errors="replace"), 1):
            if SUSPECT.match(line):
                bad.append((path, number, line.rstrip()))
    for path, number, line in bad:
        print("UNPARSEABLE %s:%d  %s" % (path, number, line.strip()))
    print("%d marker lines reccmp cannot parse" % len(bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
