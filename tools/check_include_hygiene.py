# Fails on three include faults a build does not report clearly.
#
#     python tools/check_include_hygiene.py
#     python tools/check_include_hygiene.py --list
#
#   A guard that does not close: `#ifndef X` with no `#define X` under it guards nothing.
#
#   Two headers sharing a guard macro, usually from copying a header. The second one read is
#   silently empty.
#
#   One header included twice in one file. Harmless, which is why it survives.
#
# It reads text only, so unlike tools/check_headers_standalone.py it is cheap enough for the
# battery. Read only.

import collections
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

IFNDEF = re.compile(r"^#ifndef[ \t]+(\w+)[ \t]*$", re.M)
INCLUDE = re.compile(r'^#include\s+["<]([^">]+)[">]', re.M)

SKIP = ("jpeg/",)


def sources(suffix):
    for path in sorted(glob.glob(os.path.join(ROOT, "src", "**", suffix), recursive=True)):
        shown = os.path.relpath(path, os.path.join(ROOT, "src")).replace(os.sep, "/")

        if not shown.startswith(SKIP):
            yield path, shown


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    byGuard = collections.defaultdict(list)
    open_ = []
    twice = []

    for path, shown in sources("*.h"):
        text = open(path, encoding="utf-8", errors="replace").read()
        first = IFNDEF.search(text)

        if not first:
            open_.append((shown, "has no #ifndef guard at all"))
            continue

        name = first.group(1)

        if not re.search(r"^#define[ \t]+%s[ \t]*$" % re.escape(name), text, re.M):
            open_.append((shown, "opens #ifndef %s and never defines it" % name))
            continue

        byGuard[name].append(shown)

    shared = sorted((name, where) for name, where in byGuard.items() if len(where) > 1)

    for suffix in ("*.cpp", "*.h"):
        for path, shown in sources(suffix):
            text = open(path, encoding="utf-8", errors="replace").read()

            for inc, n in collections.Counter(INCLUDE.findall(text)).items():
                if n > 1:
                    twice.append((shown, inc, n))

    if show or open_ or shared or twice:
        for shown, why in open_:
            print("GUARD    %-42s %s" % (shown, why))

        for name, where in shared:
            print("SHARED   %-42s %s" % (name, ", ".join(where)))

        for shown, inc, n in twice:
            print("TWICE    %-42s %-32s %d times" % (shown, inc, n))

    print("%d header(s) with a guard that closes, %d without, %d macro(s) two headers share, "
          "%d file(s) including one header twice"
          % (len(byGuard), len(open_), len(shared), len(twice)))

    return 1 if (open_ or shared or twice) else 0


if __name__ == "__main__":
    sys.exit(main())
