# Counts the classes and namespaces whose bodies sit in more than one .cpp, or in no .cpp named for them.
#
#   python tools/namespace_spread.py           # the summary line
#   python tools/namespace_spread.py --list    # each such qualifier, its files and body counts
#   python tools/namespace_spread.py --all     # every qualifier, spread or not
#   python tools/namespace_spread.py --free    # every body at global scope, by file
#
# tools/class_spread.py reads only marked bodies written as Name::Function at column nought. This
# reads every body in every .cpp, marked or not, by walking the braces: a function defined inside
# `namespace X { }` counts for X, and one defined inside a class body counts for the class.
# Comments, literals and preprocessor lines are blanked first, so both arms of an #if count.
#
# A qualifier is a namespace when the tree opens it with `namespace`, and a class otherwise.
# A class that one .cpp defines and no header does, and a namespace whose every body in the file is
# static, are local to that unit: they cannot leave it, so neither ratchet counts them.
#
# A file is named for a qualifier when its name, lower case and without underscores, is the
# qualifier's last part: namespace BringUp belongs in bringup.cpp. A file listed in
# symbols/layout_exceptions.csv settles its bodies, so they do not count as a spread; --list marks it.
#
# Read only. Three ratchets: exits 1 when more qualifiers are spread than CEILING, when more have
# no file named for them than UNNAMED_CEILING, or when more bodies sit at global scope than
# GLOBAL_CEILING. A game function belongs to a class or a namespace. Only the names in
# GLOBAL_NAMES may stay at global scope, and they do not count.

import collections
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Ratchets: lower them as families fold, never raise them.
CEILING = 0
UNNAMED_CEILING = 0
GLOBAL_CEILING = 0

# The bodies that must stay at global scope, and why.
GLOBAL_NAMES = {
    "WinMain": "the CRT's start-up code calls it by its unmangled name",
    "_purecall": "the bring-up build's replacement for LIBCMT's, which the linker takes by name",
}

SKIP = ("src/jpeg/",)

LEXEME = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.S)
DIRECTIVE = re.compile(r"^[ \t]*#(?:[^\n]*\\\n)*[^\n]*", re.M)
ACCESS = re.compile(r"^(?:(?:public|private|protected)\s*:\s*)+")
SCOPE = re.compile(r"^(?:template\s*<[^{]*>\s*)?(?:static\s+)?(namespace|class|struct|union|enum)\b\s*(\w*)")
NAME = re.compile(r"((?:\w+\s*::\s*)*(?:~\s*\w+|operator\s*[^\w\s(][^(]*|operator\s+\w+|\w+))\s*$")
AFTER = re.compile(r"^\s*(?:const\s*)?(?:throw\s*\([^)]*\)\s*)?(?::.*)?$", re.S)
NOT_A_FUNCTION = set("if while for switch catch return sizeof do else case new delete throw "
                     "typedef using".split())


def blank(match):
    """Keep a blanked span's line breaks so line numbers survive."""
    return re.sub(r"[^\n]", " ", match.group(0))


def code_of(path):
    text = open(path, encoding="latin-1", newline="").read().replace("\r\n", "\n")
    return DIRECTIVE.sub(blank, LEXEME.sub(blank, text))


def split_call(head):
    """The text before the first top-level parenthesis, and the text after its partner."""
    start = head.find("(")

    if start < 0:
        return None, None

    depth = 0

    for index in range(start, len(head)):
        if head[index] == "(":
            depth += 1
        elif head[index] == ")":
            depth -= 1

            if depth == 0:
                return head[:start], head[index + 1:]

    return None, None


def bodies_of(path):
    """Each body as (qualifier path, function name, line, static), the namespaces the file opens,
    and the classes it defines."""
    code = code_of(path)
    # Each open brace is one of: ("ns", name), ("class", name), ("fn", None), ("other", None).
    stack = []
    last = 0
    found = []
    opened = set()
    defined = set()

    for index, char in enumerate(code):
        if char not in "{};":
            continue

        head = " ".join(code[last:index].split())
        last = index + 1

        if char == ";":
            continue

        if char == "}":
            if stack:
                stack.pop()

            continue

        if any(kind == "fn" for kind, unused in stack):
            stack.append(("fn", None))
            continue

        head = ACCESS.sub("", head)
        scope = SCOPE.match(head)

        if scope:
            kind = scope.group(1)

            if kind == "namespace":
                stack.append(("ns", scope.group(2)))
                opened.add("::".join([name for unused, name in stack]))
            elif kind in ("class", "struct") and scope.group(2) and "(" not in head:
                stack.append(("class", scope.group(2)))
                defined.add(scope.group(2))
            else:
                stack.append(("other", None))

            continue

        name = definition(head)

        if name is None:
            stack.append(("other", None))
            continue

        outer = [scope_name for kind, scope_name in stack if kind in ("ns", "class")]
        parts = [part.strip() for part in name.split("::")]
        qualifier = "::".join(outer + parts[:-1])
        found.append((qualifier, parts[-1], code.count("\n", 0, index) + 1,
                      head.startswith("static ")))
        stack.append(("fn", None))

    return found, opened, defined


def definition(head):
    """The name a function definition's head defines, or None when the head is not one."""
    before, after = split_call(head)

    if before is None:
        return None

    if "=" in before.replace("operator", "") or not AFTER.match(after):
        return None

    words = before.split()

    if not words or words[0] in NOT_A_FUNCTION:
        return None

    hit = NAME.search(before)

    if hit is None:
        return None

    name = re.sub(r"\s+", "", hit.group(1))

    if name.split("::")[-1] in NOT_A_FUNCTION:
        return None

    return name


def key(qualifier):
    """The file name a qualifier belongs in, without its extension."""
    return qualifier.split("::")[-1].lower().replace("_", "")


def own_name(shown):
    return os.path.basename(shown)[:-4].lower().replace("_", "")


def exceptions():
    """The satellites symbols/layout_exceptions.csv settles."""
    path = os.path.join(ROOT, "symbols", "layout_exceptions.csv")
    rows = [line for line in open(path, encoding="utf-8") if line.strip()
            and not line.startswith("#")]

    return set(row.split(",", 1)[0] for row in rows[1:])


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv
    every = "--all" in sys.argv
    free = "--free" in sys.argv

    bodies = collections.defaultdict(collections.Counter)
    globals_by_file = collections.defaultdict(list)
    allowed = []
    namespaces = set()
    in_headers = set()
    # A qualifier is local to its unit when the .cpp defines the class itself and no header does,
    # or when every body under it is static. It cannot leave its unit, so neither ratchet counts it.
    local_to = collections.defaultdict(set)
    shared = set()

    for directory, unused, names in os.walk("src"):
        for name in sorted(names):
            if not name.endswith((".cpp", ".h")):
                continue

            shown = os.path.join(directory, name).replace(os.sep, "/")

            if shown.startswith(SKIP):
                continue

            found, opened, defined = bodies_of(shown)
            namespaces |= set(one.split("::")[-1] for one in opened)

            if not name.endswith(".cpp"):
                in_headers |= defined
                continue

            for qualifier, function, line, static in found:
                if not qualifier and function in GLOBAL_NAMES:
                    allowed.append(function)
                    continue

                if not qualifier:
                    globals_by_file[shown].append((line, function))
                    continue

                bodies[qualifier][shown] += 1

                if static or qualifier.split("::")[-1] in defined:
                    local_to[qualifier].add(shown)
                else:
                    shared.add(qualifier)

    listed = exceptions()
    local = set(qualifier for qualifier in bodies
                if qualifier not in shared and len(local_to[qualifier]) == 1
                and qualifier.split("::")[-1] not in in_headers)
    spread = sorted(qualifier for qualifier, files in bodies.items()
                    if len([shown for shown in files if shown not in listed]) > 1
                    and qualifier not in local)
    unnamed = sorted(qualifier for qualifier, files in bodies.items()
                     if qualifier not in local
                     and not any(own_name(shown) == key(qualifier) for shown in files)
                     and not all(shown in listed for shown in files))

    def kind(qualifier):
        if qualifier.split("::")[-1] in namespaces:
            return "namespace"

        return "class"

    for qualifier in sorted(bodies, key=lambda one: (-len(bodies[one]), one)):
        if not every and qualifier not in spread and qualifier not in unnamed:
            continue

        if not (show or every):
            break

        files = bodies[qualifier]
        print("%s %s, %d bod%s in %d file(s)%s"
              % (kind(qualifier), qualifier, sum(files.values()),
                 "y" if sum(files.values()) == 1 else "ies", len(files),
                 ", none named for it" if qualifier in unnamed else ""))

        for shown, count in sorted(files.items(), key=lambda item: (-item[1], item[0])):
            print("    %3d %s%s" % (count, shown, "  (layout exception)" if shown in listed else ""))

    if free:
        for shown in sorted(globals_by_file):
            print("%s: %s" % (shown, ", ".join(function for line, function in globals_by_file[shown])))

    classes = sum(1 for qualifier in spread if kind(qualifier) == "class")
    at_global = sum(len(found) for found in globals_by_file.values())

    print("%d of %d qualifier(s) have bodies in more than one .cpp (%d class(es), %d namespace(s)); "
          "%d with no .cpp named for them; %d local to one unit; %d bod%s at global scope in "
          "%d file(s), besides %s; ceilings %d, %d and %d"
          % (len(spread), len(bodies), classes, len(spread) - classes, len(unnamed), len(local),
             at_global,
             "y" if at_global == 1 else "ies", len(globals_by_file),
             " and ".join(sorted(allowed)) or "none", CEILING, UNNAMED_CEILING,
             GLOBAL_CEILING))

    return 1 if (len(spread) > CEILING or len(unnamed) > UNNAMED_CEILING
                 or at_global > GLOBAL_CEILING) else 0


if __name__ == "__main__":
    sys.exit(main())
