# Lists integer assignments that narrow a value without an explicit cast.
#
# `bySlot = pContext->GetActor()->GetSlot();` where bySlot is an `unsigned char` and GetSlot
# returns a `short`. The client truncates explicitly, and VC6 given the explicit form can emit
# different code, so each site is a candidate to measure. VC6 warns C4244 on only some of these,
# so the build log is not the list.
#
# Left out:
#
#   Anything touching float or double. A cast there inserts a rounding, and an extra FSTP is a
#   real risk against an x87 body.
#
#   A call whose return type is not known, or a name several classes declare with different
#   return types. The call site's class is not resolved, so an ambiguous name is skipped.
#
#   A site whose comment block above already records a measured answer.
#
# Read only. Every row is a candidate to measure, not a fix to apply.

import collections
import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# How many bytes each type occupies, for the comparison that decides "narrower".
WIDTH = {"char": 1, "signed char": 1, "unsigned char": 1, "BYTE": 1,
         "short": 2, "unsigned short": 2, "WORD": 2,
         "int": 4, "unsigned int": 4, "long": 4, "unsigned long": 4, "DWORD": 4, "BOOL": 4}

# A no-argument method declared in a header, with its return type.
DECL = re.compile(r"^[ 	]*(?:virtual\s+|static\s+|inline\s+)*"
                  r"((?:unsigned\s+|signed\s+)?\w+)\s+(?:__\w+\s+)?(\w+)\s*\(\s*\)\s*"
                  r"(?:const\s*)?[;{]", re.M)
# A local declared with one of the narrow types.
LOCAL = re.compile(r"^[ 	]*((?:unsigned\s+|signed\s+)?(?:char|short)|BYTE|WORD)\s+(\w+)\s*;", re.M)
# Any integer local or parameter, for the declarations in scope at one site.
SCOPED = re.compile(r"(?:^[ \t]*|[(,]\s*)((?:unsigned\s+|signed\s+)?(?:char|short|int|long)"
                    r"|BYTE|WORD|DWORD|BOOL)\s+(\w+)\s*(?=[;,)])", re.M)
# The first line of a function definition: flush left, and not a directive, comment or brace.
DEFINITION = re.compile(r"^[A-Za-z_][^\n;]*\(", re.M)
# An assignment of a no-argument call, with nothing casting it.
ASSIGN = re.compile(r"^[ 	]*(\w+)\s*=\s*([\w\.\->]*?)\b(\w+)\(\s*\)\s*;", re.M)
# The source need not be a call: reading a member or another local truncates the same way, and
# VC6 does not warn when both sides are integers of the same signedness.
READ = re.compile(r"^[ 	]*(\w+)\s*=\s*((?:[\w]+(?:->|\.))*)(m_\w+|\w+)\s*;", re.M)
# A member declared in a header, for the width of the right hand side above.
MEMBER = re.compile(r"^[ 	]*(?:const\s+|volatile\s+|mutable\s+)*"
                    r"((?:unsigned\s+|signed\s+)?\w+)\s+(m_\w+)\s*;", re.M)


def members(root):
    """Each member name mapped to the widths headers declare for it."""
    found = collections.defaultdict(set)

    for path in sorted(glob.glob(os.path.join(root, "**", "*.h"), recursive=True)):
        text = code(open(path, encoding="utf-8", errors="replace").read())

        for kind, name in MEMBER.findall(text):
            kind = re.sub(r"\s+", " ", kind).strip()

            if kind in WIDTH:
                found[name].add(kind)

    return found


def returns(root):
    """Each no-argument method name mapped to the return types headers declare for it."""
    found = collections.defaultdict(set)

    for path in sorted(glob.glob(os.path.join(root, "**", "*.h"), recursive=True)):
        text = code(open(path, encoding="utf-8", errors="replace").read())

        for kind, name in DECL.findall(text):
            kind = re.sub(r"\s+", " ", kind).strip()

            if kind in WIDTH:
                found[name].add(kind)

    return found


def noted(text, at):
    """True when the comment block above this offset already answers for the site.

    A measurement can go either way, so a site whose note records the answer is settled.
    """
    kept = []

    for line in reversed(text[:at].rstrip().split(chr(10))):
        if not line.strip().startswith("//"):
            break

        kept.append(line.strip()[2:].strip())

    return "narrow" in " ".join(kept).lower() or "cast" in " ".join(kept).lower()


def in_scope(text, at, narrow):
    """The integer widths in scope at an offset.

    The file's narrow locals, overridden by the enclosing function's own locals and parameters,
    so a parameter is not read as a same-named local of another function.
    """
    kinds = dict(narrow)
    start = 0

    for found in DEFINITION.finditer(text, 0, at):
        start = found.start()

    for kind, name in SCOPED.findall(text, start, at):
        kinds[name] = re.sub(r"\s+", " ", kind).strip()

    return kinds


def main():
    os.chdir(ROOT)
    quiet = "--count" in sys.argv

    declared = returns("src")
    fields = members("src")
    hits = []
    skipped = collections.Counter()

    for path in sorted(glob.glob(os.path.join("src", "**", "*.cpp"), recursive=True)):
        shown = os.path.relpath(path, "src").replace(os.sep, "/")
        raw = open(path, encoding="utf-8", errors="replace").read()
        text = code(raw)
        narrow = {}

        for kind, name in LOCAL.findall(text):
            narrow[name] = re.sub(r"\s+", " ", kind).strip()

        for hit in ASSIGN.finditer(text):
            target = hit.group(1)
            method = hit.group(3)
            kinds = in_scope(text, hit.start(), narrow)

            if WIDTH.get(kinds.get(target), 4) > 2 or method not in declared:
                continue

            if noted(raw, hit.start()):
                continue

            if len(declared[method]) > 1:
                skipped[method] += 1
                continue

            source = list(declared[method])[0]

            if WIDTH[source] <= WIDTH[kinds[target]]:
                continue

            hits.append((shown, text.count("\n", 0, hit.start()) + 1, target,
                         kinds[target], method + "()", source))

        for hit in READ.finditer(text):
            target = hit.group(1)
            name = hit.group(3)
            kinds = in_scope(text, hit.start(), narrow)

            if WIDTH.get(kinds.get(target), 4) > 2 or target == name:
                continue

            if noted(raw, hit.start()):
                continue

            # A local of the same name in this file wins over a member: it is the one in
            # scope, and it is the one whose width the assignment actually narrows.
            # Only a narrow local is a source here; an int local is out of this list's reach.
            if hit.group(2) == "" and name in kinds and WIDTH.get(kinds[name], 4) > 2:
                continue
            elif hit.group(2) == "" and name in kinds:
                source = kinds[name]
            elif name in fields and len(fields[name]) == 1:
                source = list(fields[name])[0]
            elif name in fields:
                skipped[name] += 1
                continue
            else:
                continue

            if WIDTH[source] <= WIDTH[kinds[target]]:
                continue

            hits.append((shown, text.count("\n", 0, hit.start()) + 1, target,
                         kinds[target], hit.group(2) + name, source))

    if not quiet:
        for shown, at, target, kind, method, source in hits:
            print("%-40s:%-6d %s is %s, %s is %s"
                  % (shown, at, target, kind, method, source))

    print("%d assignment(s) narrowing a value without a cast; %d skipped as ambiguous"
          % (len(hits), sum(skipped.values())))

    return 1 if hits else 0


if __name__ == "__main__":
    sys.exit(main())
