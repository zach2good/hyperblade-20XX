# Surveys how the tree uses each `void*` and `char*` global, to find which can be retyped.
#
# Usage: python tools/untyped_globals.py [--list]
#   --list  print each global with its verdict and declaration sites.
# Read only; always exits 0. tools/untyped_members.py runs the same survey over members.
#
#   ONE       every cast of the value agrees and nothing steps the global in bytes. Retype
#             every declaration and the casts go.
#   ADDRESSED the cast is on the global's address, so the storage itself is another record:
#             `((String*)&g_pszName)` means the global is a String, not a pointer to one.
#             Listed under SEVERAL.
#   SEVERAL   the casts disagree, often truthfully (two hierarchies selected by a flag); a note
#             is then the answer, not a type. Byte arithmetic also lands here: a real type
#             would scale every step (see tools/check_scaled_offsets.py).
#   NONE      nothing casts or dereferences it; its writers are printed as the only evidence.
#
# Every declaration site is listed, because globals.h, globals.cpp and local externs must all
# change together.

import collections
import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# A declaration or definition of an untyped global, in a header or a source file.
DECL = re.compile(r"^[ \t]*(?:extern\s+|static\s+)?(?:(?:const|volatile)\s+)*(void|char)"
                  r"(?:\s+(?:const|volatile)\b)*\s*((?:\*\s*(?:(?:const|volatile)\b\s*)*)+)"
                  r"(g_\w+)\s*((?:\[[^\]]*\]\s*)*)(?:=[^;]*)?;", re.M)

CAST = r"\(\s*(?:const\s+)?(?:class\s+|struct\s+)?(\w+)\s*\*+\s*\)\s*(?:\(\s*)?\b%s\b"
ADDR = r"\(\s*(?:const\s+)?(?:class\s+|struct\s+)?(\w+)\s*\*+\s*\)\s*&\s*\b%s\b"
# An array's subscripts reach its element, so only what follows them dereferences it.
DEREF = r"\b%s%s\s*(?:->|\[)"
SUBSCRIPT = r"\s*\[[^\[\]]*\]"
ARITH = r"\b%s\s*(?:\+\+|--|\+=|-=)|\b%s\s*[+-]\s*[\w(]"
ASSIGN = re.compile(r"\b(g_\w+)\s*=\s*([^=;][^;]*)")

SKIP = ("void", "char", "int", "short", "long", "unsigned", "float", "double", "const")


def sources(root):
    return (sorted(glob.glob(os.path.join(root, "**", "*.h"), recursive=True))
            + sorted(glob.glob(os.path.join(root, "**", "*.cpp"), recursive=True)))


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    paths = sources("src")
    bodies = {}
    declared = collections.defaultdict(list)
    rank = collections.Counter()

    for path in paths:
        shown = os.path.relpath(path, "src").replace(os.sep, "/")
        bodies[path] = code(open(path, encoding="utf-8", errors="replace").read())

        for kind, stars, name, dims in DECL.findall(bodies[path]):
            declared[name].append((shown, kind + "*" * (stars.count("*") - 1)))
            rank[name] = max(rank[name], dims.count("["))

    one = []
    several = []
    none = []
    writes = collections.defaultdict(list)

    for name in sorted(declared):
        casts = collections.Counter()
        addressed = collections.Counter()
        deref = 0
        steps = 0

        for path in paths:
            body = bodies[path]

            if name not in body:
                continue

            for hit in re.findall(ADDR % re.escape(name), body):
                if hit not in SKIP:
                    addressed[hit] += 1

            # Exclude the address form, or `(String*)&g_p` also counts as a value cast.
            for hit in re.findall(CAST % re.escape(name), body.replace("&" + name, " ")):
                if hit not in SKIP:
                    casts[hit] += 1

            deref += len(re.findall(DEREF % (re.escape(name), SUBSCRIPT * rank[name]), body))
            steps += len(re.findall(ARITH % (re.escape(name), re.escape(name)), body))

            if path.endswith(".cpp"):
                shown = os.path.relpath(path, "src").replace(os.sep, "/")

                for at, line in enumerate(body.split("\n"), 1):
                    for hit in ASSIGN.finditer(line):
                        if hit.group(1) == name and hit.group(2).strip() not in ("0", "0;"):
                            writes[name].append("%s:%d  %s"
                                                % (shown, at, hit.group(2).strip()[:56]))

        where = ", ".join("%s(%s*)" % (shown, kind) for shown, kind in declared[name])

        if addressed and not casts:
            several.append((name, where, addressed,
                            "ADDRESSED: the storage is read as this record's head"))
        elif len(casts) == 1 and not steps:
            one.append((name, where, list(casts)[0], casts[list(casts)[0]], len(declared[name])))
        elif len(casts) == 1:
            several.append((name, where, casts, "stepped in bytes at %d site(s)" % steps))
        elif len(casts) > 1:
            several.append((name, where, casts, ""))
        elif not deref:
            none.append((name, where))

    if show:
        for name, where, kind, n, sites in one:
            print("ONE      %-26s %s*, %d site(s), %d declaration(s)\n           %s"
                  % (name, kind, n, sites, where))
        for name, where, casts, note in several:
            print("SEVERAL  %-26s %s%s\n           %s"
                  % (name, ", ".join("%s(%d)" % (k, v) for k, v in casts.most_common()),
                     "   " + note if note else "", where))
        for name, where in none:
            print("NONE     %-26s %s" % (name, where))

            for site in writes.get(name, [])[:3]:
                print("           writes  %s" % site)

    print("%d untyped global(s): %d cast to one type everywhere, %d to several, "
          "%d never cast or dereferenced"
          % (len(declared), len(one), len(several), len(none)))

    return 0


if __name__ == "__main__":
    sys.exit(main())
