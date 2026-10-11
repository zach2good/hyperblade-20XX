# Finds a cast of a call's result to the type that call already returns, pointer or scalar.
#
# Usage: python tools/redundant_return_casts.py [--ambiguous]
#   --ambiguous  list the method names skipped because two classes declare them differently.
# Read only. Exits 1 when any redundant cast remains. Run it after typing a return value.
#
# `(EngineObject*)pVm->GetActor()` where GetActor is declared `virtual EngineObject* GetActor()`.
# A cast between a type and itself emits nothing, so removing it is free.
#
# Not reported:
#   - A cast to a base or derived type. It is a real conversion. Only an exact match counts.
#   - A name two classes declare with different return types. The call site's class is not
#     known here, so it is skipped and listed under --ambiguous.
#   - A cast that adds or drops const. It is a real conversion.

import collections
import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# A method declared in a header returning a pointer. The const is captured as part of the type.
# `^[ \t]*`, not `^\s*`: a whitespace class matches a newline and would run across lines.
DECL = re.compile(
    r"^[ \t]*(?:virtual\s+|static\s+|inline\s+)*(const\s+)?(?:class\s+|struct\s+)?"
    r"(\w+)\s*\*\s*(?:__\w+\s+)?(\w+)\s*\([^;{}]*\)\s*(?:const\s*)?[;{]", re.M)
# The cast of such a call at a use site. The argument list is matched to one level of nesting.
USE = re.compile(
    r"\(\s*(const\s+)?(?:class\s+|struct\s+)?(\w+)\s*\*\s*\)\s*"
    r"([\w\.\->]+)->(\w+)\(\s*(?:[^()]|\([^()]*\))*\)")

# Only void is skipped: a cast to void* is how this tree spells deliberate type erasure.
SKIP = ("void",)

# A function declared in a header returning one of these, and a cast of a qualified call to one.
# An unqualified call is left out: a Windows function can share the name of a member.
SCALARS = r"unsigned\s+short|unsigned\s+char|unsigned\s+int|short|char|int"
SCALAR_KINDS = ("unsigned short", "unsigned char", "unsigned int", "short", "char", "int")
SCALAR_DECL = re.compile(
    r"^[ \t]*(?:virtual\s+|static\s+|inline\s+)*(" + SCALARS + r")\s+(?:__\w+\s+)?(\w+)\s*\([^;{}]*\)"
    r"\s*(?:const\s*)?(?:=\s*0\s*)?[;{]", re.M)
SCALAR_USE = re.compile(
    r"\(\s*(" + SCALARS + r")\s*\)\s*((?:this\s*->\s*|[A-Za-z_]\w*(?:->|\.|::))+)([A-Z]\w*)\(")


def returns(root):
    """Every method name in the headers, mapped to the return types declared, const and all."""
    found = collections.defaultdict(set)

    for path in sorted(glob.glob(os.path.join(root, "**", "*.h"), recursive=True)):
        text = code(open(path, encoding="utf-8", errors="replace").read())

        for isConst, kind, name in DECL.findall(text):
            if kind not in SKIP:
                found[name].add((bool(isConst), kind))

    return found


def scalar_returns(root, pointers):
    """Every function name declared to return a scalar, unless it is also declared otherwise."""
    found = collections.defaultdict(set)

    for path in sorted(glob.glob(os.path.join(root, "**", "*.h"), recursive=True)):
        text = code(open(path, encoding="utf-8", errors="replace").read())

        for kind, name in SCALAR_DECL.findall(text):
            found[name].add(" ".join(kind.split()))

    return dict((name, kinds) for name, kinds in found.items() if name not in pointers)


def main():
    os.chdir(ROOT)
    show = "--ambiguous" in sys.argv

    declared = returns("src")
    scalars = scalar_returns("src", declared)
    hits = []
    skipped = collections.Counter()

    for path in sorted(glob.glob(os.path.join("src", "**", "*.cpp"), recursive=True)):
        shown = os.path.relpath(path, "src").replace(os.sep, "/")
        raw = open(path, encoding="utf-8", errors="replace").read()
        text = code(raw)

        for at, line in enumerate(text.split("\n"), 1):
            for isConst, kind, target, name in USE.findall(line):
                # The const must match as well as the type.
                if name not in declared or (bool(isConst), kind) not in declared[name]:
                    continue

                if len(declared[name]) > 1:
                    skipped[name] += 1
                    continue

                hits.append((shown, at, kind, target, name))

            for kind, target, name in SCALAR_USE.findall(line):
                kind = " ".join(kind.split())

                if name not in scalars or kind not in scalars[name]:
                    continue

                if len(scalars[name]) > 1:
                    skipped[name] += 1
                    continue

                hits.append((shown, at, kind, target, name))

    if show:
        for name, n in skipped.most_common():
            print("AMBIGUOUS %-30s %d site(s), returns %s"
                  % (name, n, "/".join("%s%s" % ("const " if c else "", k)
                                       for c, k in sorted(declared[name]))))
    else:
        counts = collections.Counter()

        for shown, at, kind, target, name in hits:
            counts[(shown, name, kind)] += 1

        for (shown, name, kind), n in counts.most_common():
            print("%-44s %-28s cast to %s, %d site(s)" % (shown, name + "()", kind if kind in SCALAR_KINDS else kind + "*", n))

    print("%d cast(s) of a call to the type it already returns, over %d name(s); "
          "%d site(s) skipped as ambiguous"
          % (len(hits), len(set(h[4] for h in hits)), sum(skipped.values())))

    return 1 if hits else 0


if __name__ == "__main__":
    sys.exit(main())
