# Finds an `#include` whose header introduces no name the including file writes.
#
# Usage: python tools/unused_includes.py [--list] [--shadowed] [--headers] [--apply FILE]
#   --list      print each unused include.
#   --shadowed  report only includes another include already pulls in.
#   --headers   check headers instead of .cpp files.
#   --apply F   remove the unused includes from file F (a path under src/).
# Exits 1 when any unused include remains. Read only without --apply.
#
# A header's names are its classes, structs, unions, enums, enumerators, defines, typedefs,
# externs, members and column 0 function declarations, plus everything it includes. A header
# that reaches a system include cannot be enumerated and is never reported. A header that holds
# nothing but its guard and includes exists to include them, so --headers skips it.
#
# Two kinds of answer:
#   SHADOWED  another include already pulls this header in, so removing the line changes nothing.
#   DIRECT    it does not. Removal can still break a parse (`void f(Foo*)` in another header
#             needs this one's `class Foo;`), so apply one file at a time and build.
#
# A delete needs the full type: `delete g_pAudioMgr;` without the class definition compiles to a
# plain operator delete with no virtual destructor call (warning C4150). So each deleted pointer's
# declared type counts as a use. Removing an include from a header takes it away from every
# consumer; tools/fixincludes.py adds it back where a consumer needs it.

import collections
import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# A ratchet: do not raise it.
BUDGET = 0

INCLUDE = re.compile(r'^#include\s+"([^"]+)"', re.M)
# A system include, whose names this cannot enumerate.
SYSTEM = re.compile(r"^#include\s+<", re.M)

INTRODUCES = (
    re.compile(r"^(?:class|struct|union)\s+(?:__declspec\s*\([^)]*\)\s*)?(\w+)", re.M),
    re.compile(r"^\s*#\s*define\s+(\w+)", re.M),
    re.compile(r"^(?:enum)\s+(\w+)", re.M),
    re.compile(r"^\s*typedef\s+[\w\s:*&<>\[\],()]*?(\w+)\s*(?:\[[^\]]*\])?\s*;", re.M),
    re.compile(r"^\s*extern\s+(?:\"C\"\s+)?[\w\s:*&<>]*?(\w+)\s*(?:\[[^\]]*\])?\s*(?:;|\()", re.M),
    re.compile(r"^[\w:*&<>\[\] ]*?\b(\w+)\s*\(", re.M),
    re.compile(r"^\s{2,}([A-Z][A-Z0-9_]{2,})\s*(?:=|,|$)", re.M),
    # Members count too: `g_gameApp.m_strExeDir` needs GameApp's definition without writing
    # `GameApp`.
    re.compile(r"^[ \t]+(?:virtual\s+|static\s+|inline\s+|const\s+|mutable\s+)*"
               r"[\w:*&<>\[\] ]*?\b(\w+)\s*(?:\(|;|\[|=)", re.M),
)

# Headers (paths under src/) to skip because a consumer reaches another header through them.
REACHED_THROUGH = {}

KEYWORDS = set("if else for while switch return sizeof class struct union enum typedef extern "
               "const static inline virtual public private protected namespace template "
               "operator new delete throw try catch do goto break continue case default "
               "signed unsigned void int char short long float double bool true false this "
               "defined include define ifndef endif pragma".split())


# `delete p;` and `delete[] p;`, where p is all the file names.
DELETES = re.compile(r"\bdelete\s*(?:\[\s*\])?\s*(?:\(\s*\w+\s*\*\s*\)\s*)?"
                     r"([A-Za-z_][\w>.\-]*)")
# Where a pointer's type is written down, wherever it is written down.
POINTER = re.compile(r"^[ \t]*(?:extern\s+|static\s+|const\s+|volatile\s+|mutable\s+"
                     r"|struct\s+|class\s+)*([A-Za-z_]\w*)\s*\*\s*(?:const\s+)?(\w+)"
                     r"\s*(?:;|=|\[)", re.M)


def headers():
    """Each header's introduced names, what it pulls in, and whether it reaches <system>."""
    names = {}
    pulls = {}
    raw = {}
    opaque = {}

    for path in glob.glob(os.path.join(ROOT, "src", "**", "*.h"), recursive=True):
        shown = os.path.relpath(path, os.path.join(ROOT, "src")).replace(os.sep, "/")
        text = code(open(path, encoding="utf-8", errors="replace").read())
        found = set()

        for pattern in INTRODUCES:
            for name in pattern.findall(text):
                if name and name not in KEYWORDS and len(name) > 2:
                    found.add(name)

        names[shown] = found
        raw[shown] = [inc.replace("\\", "/") for inc in INCLUDE.findall(text)]
        opaque[shown] = SYSTEM.search(text) is not None

    # An include can be relative to the header that writes it: planvm/planvm.h's
    # `#include "planstate.h"` is planvm/planstate.h.
    for shown, incs in raw.items():
        here = os.path.dirname(shown)
        pulls[shown] = []

        for inc in incs:
            beside = "%s/%s" % (here, inc) if here else inc
            pulls[shown].append(inc if inc in names else
                                beside if beside in names else inc)

    return names, pulls, opaque


def reaches(pulls, start, seen=None):
    if seen is None:
        seen = set()

    for nxt in pulls.get(start, ()):
        if nxt in seen:
            continue

        seen.add(nxt)
        reaches(pulls, nxt, seen)

    return seen


def wholeOf(names, pulls, start, cache):
    """Everything a header introduces, including what it pulls in."""
    if start in cache:
        return cache[start]

    cache[start] = set(names.get(start, ()))

    for nxt in reaches(pulls, start):
        cache[start] |= names.get(nxt, set())

    return cache[start]


def deleted(text, types):
    """The TYPE of every pointer the file deletes, which needs its definition in scope."""
    found = set()

    for name in DELETES.findall(text):
        bare = name.split("->")[-1].split(".")[-1]

        if bare in types:
            found |= types[bare]

    return found


def pointerTypes(text):
    """Pointer name to declared type, from this file and every header in the tree."""
    found = collections.defaultdict(set)

    for kind, name in POINTER.findall(text):
        if kind not in KEYWORDS:
            found[name].add(kind)

    return found


def unused(names, pulls, opaque, path, cache, types):
    """(header, how many names it would have given, whether another include covers it)."""
    text = code(open(path, encoding="utf-8", errors="replace").read())
    used = set(re.findall(r"\b\w+\b", INCLUDE.sub("", text)))

    local = pointerTypes(text)

    for name in local:
        types.setdefault(name, set())

    used |= deleted(text, dict((k, v | local.get(k, set())) for k, v in types.items()))
    includes = [inc.replace("\\", "/") for inc in INCLUDE.findall(text)]
    found = []

    for inc in includes:
        whole = wholeOf(names, pulls, inc, cache)

        # A header that reaches a system header cannot be enumerated, so it is never reported.
        if opaque.get(inc) or any(opaque.get(step)
                                  for step in reaches(pulls, inc)):
            continue

        if inc not in names or not whole or (whole & used):
            continue

        others = set()

        for second in includes:
            if second != inc:
                others.add(second)
                others |= reaches(pulls, second)

        found.append((inc, len(whole), inc in others))

    return found


def umbrella(path):
    """Whether a header holds nothing but its guard and includes: it exists to include them."""
    text = code(open(path, encoding="utf-8", errors="replace").read())
    rest = [line for line in text.splitlines() if line.strip()
            and not re.match(r"\s*#\s*(?:include|ifndef|define|endif)\b", line)]

    return not rest


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv
    only = "--shadowed" in sys.argv
    inHeaders = "--headers" in sys.argv
    apply = None

    if "--apply" in sys.argv:
        apply = sys.argv[sys.argv.index("--apply") + 1].replace("\\", "/")

    names, pulls, opaque = headers()
    cache = {}

    types = collections.defaultdict(set)

    for path in glob.glob(os.path.join(ROOT, "src", "**", "*.h"), recursive=True):
        for name, kinds in pointerTypes(
                code(open(path, encoding="utf-8", errors="replace").read())).items():
            types[name] |= kinds
    shadowed = 0
    direct = 0
    byFile = collections.Counter()

    where = "*.h" if inHeaders else "*.cpp"

    for path in sorted(glob.glob(os.path.join(ROOT, "src", "**", where), recursive=True)):
        shown = os.path.relpath(path, ROOT).replace(os.sep, "/")

        if shown.startswith("src/jpeg/"):
            continue

        if shown[len("src/"):] in REACHED_THROUGH:
            continue

        if inHeaders and umbrella(path):
            continue

        found = unused(names, pulls, opaque, path, cache, types)

        if only:
            found = [row for row in found if row[2]]

        if not found:
            continue

        shadowed += len([row for row in found if row[2]])
        direct += len([row for row in found if not row[2]])
        byFile[shown] = len(found)

        if show:
            for inc, n, covered in found:
                print("%-8s %-40s %-34s %d name(s)"
                      % ("SHADOWED" if covered else "DIRECT", shown, inc, n))

        if apply and shown == apply:
            raw = open(path, encoding="utf-8", newline="").read()
            nl = "\r\n" if "\r\n" in raw else "\n"

            for inc, n, covered in found:
                line = '#include "%s"%s' % (inc, nl)

                if raw.count(line) != 1:
                    print("%s: %d copies of the line for %s, left alone"
                          % (shown, raw.count(line), inc))
                    continue

                raw = raw.replace(line, "", 1)

            open(path, "w", encoding="utf-8", newline="").write(raw)
            print("%s: %d include(s) removed" % (shown, len(found)))

            return 0

    if apply:
        print("%s has no unused include, or is not a source this reads" % apply)

        return 1

    print("%d include(s) introduce no name their %s writes, over %d file(s): "
          "%d shadowed by another include and %d not"
          % (shadowed + direct, "header" if inHeaders else ".cpp",
             len(byFile), shadowed, direct))

    return 1 if shadowed + direct > BUDGET else 0


if __name__ == "__main__":
    sys.exit(main())
