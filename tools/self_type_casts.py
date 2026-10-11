# Find casts of a variable to the type it is already declared as.
#
# `(char*)pszName` where the function declares `char* pszName;` emits nothing. This is the
# variable counterpart of tools/redundant_return_casts.py. It reads locals, parameters, members
# and globals, innermost scope first: a body's declaration shadows a parameter, and either shadows
# a member or a global. A member is read against the class the body belongs to, then the tree.
# A global, or a member its owning class does not declare, needs exactly one declaration in the
# tree; names two declarations disagree on are counted as skipped.
#
# Not reported: a cast that adds or drops const, and any volatile-declared variable.
#
#     python tools/self_type_casts.py           # totals only
#     python tools/self_type_casts.py --list    # every site and every skipped name
#
# Read only. Exits 1 when it finds a cast.

import collections
import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

DECL = re.compile(r"^[ \t]*(?:class\s+|struct\s+)?(\w+)\*\s+(\w+)\s*;", re.M)
# A parameter is declared in the signature. The width words are part of the type, so
# `unsigned char* p` is not read as a char*.
SIGNATURE = re.compile(r"^[\w:~*&<>\[\] ]*?\b\w+::(?:~?\w+)\s*\(([^;{}]*)\)[^;{}]*\n?\{", re.M)
PARAM = re.compile(r"(?:const\s+|class\s+|struct\s+)*"
                   r"((?:unsigned\s+|signed\s+)?[\w:]+)\s*\*\s*(\w+)\s*(?:,|$)")
# A member declared in a header, gathered per declaring class and tree-wide.
MEMBER = re.compile(r"^[ \t]*(?:const\s+|volatile\s+|mutable\s+|struct\s+|class\s+)*"
                    r"([\w:]+)\s*\*\s*(m_\w+)\s*;", re.M)
# A definition at column 0; the trailing `;` rules out a forward declaration.
OPENER = re.compile(r"^(?:class|struct)\s+(\w+)\b[^;\r\n]*$", re.M)
# The extern lives in globals.h and some .cpp files carry their own, so sources are read too.
GLOBAL = re.compile(r"^[ \t]*(?:extern\s+)?(?:const\s+|volatile\s+)*"
                    r"(?:class\s+|struct\s+)?([\w:]+)\s*\*\s*(g_\w+)\s*(?:=[^;]*)?;", re.M)
# The cast must bind to the variable. In `(T*)p->f()` the cast applies to the call's result.
CAST = re.compile(r"\(\s*(?:class\s+|struct\s+)?(\w+)\s*\*\s*\)\s*(\w+)\b(?!\s*(?:->|\.|\[|\())")

SKIP = ("return", "sizeof", "case", "new", "delete")


def bodies(text):
    # A definition opens at column 0 and nested blocks are indented, so a leading space would
    # read every `if` or `for` as a function.
    for hit in re.finditer(r"^[\w:~*&<>\[\]][\w:~*&<>\[\] ]*\([^;{}]*\)[^;{}]*\n?\{",
                           text, re.M):
        at = text.index("{", hit.end() - 1)
        depth = 0

        for end in range(at, len(text)):
            if text[end] == "{":
                depth += 1
            elif text[end] == "}":
                depth -= 1

                if depth == 0:
                    break

        params = {}
        signature = SIGNATURE.match(text, hit.start())

        if signature:
            for kind, name in PARAM.findall(signature.group(1)):
                params[name] = re.sub(r"\s+", " ", kind).strip()

        owner = re.search(r"(\w+)::", hit.group(0))

        yield (text[hit.start():end], text.count("\n", 0, hit.start()) + 1, params,
               owner.group(1) if owner else "")


def memberTypes(root):
    """The per-class member types, the tree-wide ones, and the names left ambiguous.

    A name two headers declare with different types is dropped rather than guessed at, and the
    drop is reported, because a silent skip reads like a clean run.
    """
    found = collections.defaultdict(set)
    owned = collections.defaultdict(dict)

    for path in (sorted(glob.glob(os.path.join(root, "**", "*.h"), recursive=True))
                 + sorted(glob.glob(os.path.join(root, "**", "*.cpp"), recursive=True))):
        text = code(open(path, encoding="utf-8", errors="replace").read())
        opens = [(hit.start(), hit.group(1)) for hit in OPENER.finditer(text)]

        for pattern in (MEMBER, GLOBAL):
            for hit in pattern.finditer(text):
                kind = hit.group(1)
                name = hit.group(2)

                found[name].add(kind)

                if not name.startswith("m_"):
                    continue

                here = ""

                for start, opener in opens:
                    if start > hit.start():
                        break

                    here = opener

                # A class that declares one name twice under two types is not resolvable.
                if owned[here].get(name, kind) != kind:
                    owned[here][name] = None
                else:
                    owned[here][name] = kind

    settled = dict((name, list(kinds)[0])
                   for name, kinds in found.items() if len(kinds) == 1)
    dropped = set(name for name, kinds in found.items() if len(kinds) > 1)

    return owned, settled, dropped


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv
    owned, fields, dropped = memberTypes("src")

    hits = []
    blind = collections.Counter()

    for path in sorted(glob.glob(os.path.join("src", "**", "*.cpp"), recursive=True)):
        shown = os.path.relpath(path, "src").replace(os.sep, "/")
        text = code(open(path, encoding="utf-8", errors="replace").read())

        for body, at, params, owner in bodies(text):
            declared = {}

            for kind, name in DECL.findall(body):
                if kind in SKIP:
                    continue

                # A name declared twice in one body under two types is not resolvable here.
                if name in declared and declared[name] != kind:
                    declared[name] = None
                else:
                    declared[name] = kind

            for kind, name in CAST.findall(body):
                # Innermost first: a body's own declaration shadows a parameter, either
                # shadows a member, and the owning class's own member shadows the tree.
                settled = (declared.get(name) or params.get(name)
                           or owned.get(owner, {}).get(name) or fields.get(name))

                if settled == kind:
                    hits.append((shown, at, kind, name))
                elif settled is None and name in dropped:
                    blind[name] += 1

    counts = collections.Counter()

    for shown, at, kind, name in hits:
        counts[(shown, name, kind)] += 1

    if show:
        for (shown, name, kind), n in counts.most_common():
            print("%-44s %-22s cast to %s*, %d site(s)" % (shown, name, kind, n))

    if show:
        for name, n in blind.most_common():
            print("%-44s %-22s SKIPPED, two declarations disagree, %d cast(s)"
                  % ("", name, n))

    print("%d cast(s) of a local, parameter, member or global to the type it is "
          "declared as, "
          "over %d name(s); %d cast(s) on %d name(s) two declarations give different types"
          % (len(hits), len(counts), sum(blind.values()), len(blind)))

    return 1 if hits else 0


if __name__ == "__main__":
    sys.exit(main())
