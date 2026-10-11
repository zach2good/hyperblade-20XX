# Reports a #define that spells one fact in several files at once.
#
# Each copy is a place the fact can drift. Two kinds are reported:
#
#   DUPLICATED   the same name and the same value in several files. Move it to the header that
#                declares what it describes and delete the copies.
#   CONFLICTING  the same name with different values somewhere in the tree. It cannot be
#                consolidated until one meaning is renamed, and a reader cannot see the
#                difference.
#
# A conflict whose values are all below 0x10 is not reported, unless the name claims to be a
# sentinel or a bound: those small names are argument slots, dialog ids and counts, which vary
# by opcode on purpose. Function-like macros are compared by their bodies as written.
#
# The threshold is low for conflicts and higher for copies: two files agreeing is ordinary, and
# five is a fact that has escaped its home.

import collections
import os
import re
import sys

# A negative value such as `#define X (-1)` is a value.
DEFINE = re.compile(
    r"^#define ([A-Z][A-Z0-9_]*) +\(?(-?(?:0x[0-9a-fA-F]+|\d+))\)?\s*$", re.M)

# A name that claims to be a sentinel or a bound is reported whatever its magnitude.
SENTINEL_SUFFIX = ("_NONE", "_LIMIT", "_INVALID", "_UNSET", "_MAX", "_MIN")

# A function-like macro spelled in several files. Bodies are compared as written. A macro the
# tree deliberately repeats per translation unit, like the bring-up TRACE pair, is listed for a
# reader to judge rather than failed on.
MACRO = re.compile(r"^#define ([A-Z][A-Z0-9_]*)\(([^)]*)\)\s+(.+?)\s*$", re.M)

# Third-party and platform names, which are not this tree's to consolidate.
SKIP_PREFIX = ("JPEG_", "MAXJ", "JPOOL", "DCTSIZE", "RGB_", "CENTERJ", "D3D", "DD", "WM_",
               "VK_", "MAX_", "PATH_", "FILE_")

COPIES_AT = 5


def sourceNames(root):
    # Walk subdirectories; names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def collect(root):
    byvalue = collections.defaultdict(set)
    bymacro = collections.defaultdict(set)
    byname = collections.defaultdict(set)

    for name in sorted(sourceNames(root)):
        if not name.endswith((".cpp", ".h")):
            continue

        text = open(os.path.join(root, name), encoding="utf-8", errors="replace").read()

        for macro, args, body in MACRO.findall(text):
            if macro.startswith(SKIP_PREFIX):
                continue

            bymacro[(macro, args, body)].add(name)
        for macro, value in DEFINE.findall(text):
            if macro.startswith(SKIP_PREFIX):
                continue

            byvalue[(macro, int(value, 0))].add(name)
            byname[macro].add(int(value, 0))

    return byvalue, byname, bymacro


def main():
    root = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "src")
    show = "--list" in sys.argv

    byvalue, byname, bymacro = collect(root)

    conflicts = sorted(n for n, vals in byname.items()
                       if len(vals) > 1
                       and (max(vals) >= 0x10 or n.endswith(SENTINEL_SUFFIX)))
    copies = sorted(((len(f), n, v) for (n, v), f in byvalue.items() if len(f) >= COPIES_AT),
                    reverse=True)
    macros = sorted(((len(f), n, a, b) for (n, a, b), f in bymacro.items()
                     if len(f) >= COPIES_AT), reverse=True)

    if show:
        for macro in conflicts:
            print("CONFLICT  %-28s %s" % (macro, ", ".join("0x%x" % v for v in sorted(byname[macro]))))
        for count, macro, args, body in macros:
            print("MACRO  %2d %s(%s) %s" % (count, macro, args, body[:56]))
        for count, macro, value in copies:
            print("COPIES %2d %-28s 0x%x" % (count, macro, value))

    print("%d offset or sentinel name(s) defined with more than one value, %d defined identically in "
          "%d or more files, %d macro(s) spelled identically in as many"
          % (len(conflicts), len(copies), COPIES_AT, len(macros)))

    return 0


if __name__ == "__main__":
    sys.exit(main())
