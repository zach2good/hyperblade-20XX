# Find casts of `this` to a class the enclosing class already derives from.
#
# `((Point*)this)->Point::Zero()` inside `struct TexturePoint : public Point`. The conversion to
# a base is implicit, so the cast emits nothing. tools/self_type_casts.py and
# tools/redundant_return_casts.py find the same residue on locals and calls, and
# tools/check_self_casts.py covers casts of `this` to its own class.
#
# Not reported: casts to scalars such as `(char*)this` (see docs/RE_CRUFT.md), downcasts, and
# casts whose enclosing class is unknown.
#
# Two answers:
#
#   QUALIFIED   the call already names the base: `((Texture*)this)->Texture::Draw()`. The
#               whole `((T*)this)->` can go.
#   NAMED       the call is unqualified, so the cast may be what selects the base's member.
#               If the derived class declares a member of that name, dropping the cast calls
#               the derived one, possibly recursing. Check each; the member is printed.
#
#     python tools/base_this_casts.py           # totals only
#     python tools/base_this_casts.py --list    # every site
#
# Read only. Exits 1 when it finds a QUALIFIED cast.

import collections
import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

CAST = re.compile(r"\(\s*(?:const\s+)?(?:class\s+|struct\s+)?(\w+)\s*\*\s*\)\s*this\b")
DERIVES = re.compile(r"^\s*(?:class|struct)\s+(?:__declspec\(\w+\)\s+)?(\w+)\s*:\s*"
                     r"(?:public\s+|protected\s+|private\s+)?(\w+)", re.M)
# A definition whose qualified name gives the class, or an inline body inside one.
DEFN = re.compile(r"^[\w:~*&<>\[\] ]*?\b(\w+)::(~?\w+)\s*\([^;{}]*\)[^;{}]*\n?\{", re.M)
# The tail must not cross a line: a negated class matches a newline, and `[^;]*$` would run to
# the next semicolon anywhere in the file.
OPEN = re.compile(r"^\s*(?:class|struct)\s+(?:__declspec\(\w+\)\s+)?(\w+)\b[^;\r\n]*$", re.M)


def parents(paths):
    """Each class mapped to the one it derives from."""
    found = {}

    for path in paths:
        if not path.endswith(".h"):
            continue

        text = code(open(path, encoding="utf-8", errors="replace").read())

        for child, parent in DERIVES.findall(text):
            found.setdefault(child, parent)

    return found


def ancestors(cls, parent):
    """Every class this one derives from, bounded so a cycle cannot spin."""
    found = []
    at = cls

    while at in parent and len(found) < 16:
        at = parent[at]
        found.append(at)

    return found


def owners(text):
    """Each offset's enclosing class: a qualified definition, or a class body it sits inside."""
    found = []

    for hit in DEFN.finditer(text):
        at = text.index("{", hit.end() - 1)
        depth = 0

        for end in range(at, len(text)):
            if text[end] == "{":
                depth += 1
            elif text[end] == "}":
                depth -= 1

                if depth == 0:
                    break

        found.append((hit.start(), end, hit.group(1)))

    for hit in OPEN.finditer(text):
        brace = text.find("{", hit.end())

        if brace < 0:
            continue

        depth = 0

        for end in range(brace, len(text)):
            if text[end] == "{":
                depth += 1
            elif text[end] == "}":
                depth -= 1

                if depth == 0:
                    break

        found.append((brace, end, hit.group(1)))

    return found


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    paths = sorted(glob.glob(os.path.join("src", "**", "*.h"), recursive=True)
                   + glob.glob(os.path.join("src", "**", "*.cpp"), recursive=True))
    parent = parents(paths)
    hits = []
    skipped = collections.Counter()

    for path in paths:
        shown = os.path.relpath(path, "src").replace(os.sep, "/")
        text = code(open(path, encoding="utf-8", errors="replace").read())

        if "this" not in text:
            continue

        spans = owners(text)

        for hit in CAST.finditer(text):
            kind = hit.group(1)
            # The innermost enclosing class wins, so a method inside a class body is read as
            # that class and not as the file's first one.
            cls = ""
            width = None

            for start, end, name in spans:
                if start <= hit.start() <= end and (width is None or end - start < width):
                    cls = name
                    width = end - start

            if not cls:
                skipped["no enclosing class"] += 1
                continue

            if kind not in ancestors(cls, parent):
                continue

            # What follows the cast: `)->Base::name` is qualified, `)->name` is not. The
            # whitespace is normalised first because the call is often split over two lines.
            after = re.sub(r"\s+", "", text[hit.end():hit.end() + 120])
            qualified = re.match(r"\)?->%s::" % re.escape(kind), after) is not None
            # Past the qualifier when there is one, so the member reported is the member and
            # not the base's name a second time.
            named = re.match(r"\)?->(?:%s::)?(~?\w+)" % re.escape(kind), after)

            hits.append((shown, text.count("\n", 0, hit.start()) + 1, cls, kind, qualified,
                         named.group(1) if named else "?"))

    free = [row for row in hits if row[4]]
    read = [row for row in hits if not row[4]]

    if show:
        for shown, at, cls, kind, unused, member in free:
            print("QUALIFIED %-40s:%-6d %s->%s::%s, the cast selects nothing"
                  % (shown, at, cls, kind, member))
        for shown, at, cls, kind, unused, member in read:
            print("NAMED     %-40s:%-6d %s casts this to %s to call %s"
                  % (shown, at, cls, kind, member))

    print("%d cast(s) of this to a class it already derives from: %d qualified and free, "
          "%d naming a member that may be shadowed; %d site(s) skipped"
          % (len(hits), len(free), len(read), sum(skipped.values())))

    return 1 if free else 0


if __name__ == "__main__":
    sys.exit(main())
