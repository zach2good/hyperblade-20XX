# Find member functions whose whole body is one return of a member, and which are not const.
#
#     python tools/const_accessors.py                     # count; exit 1 if any
#     python tools/const_accessors.py --list              # name them
#     python tools/const_accessors.py --apply src/x.cpp   # add const to one file's candidates
#
# `const` on a member function changes no emitted byte. A virtual is never a candidate: a const
# override declares a second slot instead of overriding. A static has no `this`. Both keywords
# are read from the declaration, because a .cpp definition carries neither. The body must call
# nothing, because a call is const only if its callee is.
#
# --apply rewrites both the declaration and the definition, because VC6 otherwise treats them as
# two functions and the link fails.

import collections
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# A definition in a .cpp: return type, Class::Name(args), then a brace on its own line.
OUT_OF_LINE = re.compile(
    r"^(?P<head>[A-Za-z_][\w:<>,\*&\s]*?\b(?P<cls>[A-Z]\w*)::(?P<fn>\w+)\s*\((?P<args>[^)]*)\))"
    r"(?P<tail>\s*)\r?\n\{\r?\n(?P<body>[^{}]*?)\r?\n\}", re.M)

# An accessor written inside the class: `T Name(args) { return expr; }` on one line.
INLINE = re.compile(
    r"^(?P<lead>[ \t]*)(?P<head>(?!return\b|if\b|for\b|while\b)[A-Za-z_][\w:<>,\*&\s]*?"
    r"\b(?P<fn>\w+)\s*\((?P<args>[^)]*)\))\s*\{\s*return\s+(?P<expr>[^;{}]+);\s*\}", re.M)

READS = re.compile(r"^return\s+[^;]*;$")

# Returning a member's address cannot go const without changing the return type: in a const
# member `&m_pWidget018` is a `Widget* const*`, not a `Widget**`.
ADDRESS = re.compile(r"^\s*&")

SKIP = ("jpeg/",)


def sources(suffix):
    for path in sorted(glob.glob(os.path.join(ROOT, "src", "**", suffix), recursive=True)):
        shown = os.path.relpath(path, os.path.join(ROOT, "src")).replace(os.sep, "/")

        if not shown.startswith(SKIP):
            yield path, "src/" + shown


def reads_only(body):
    """One return of an expression that calls nothing and writes nothing."""
    lines = [one.strip() for one in body.splitlines() if one.strip()]

    if len(lines) != 1 or not READS.match(lines[0]):
        return False

    return expression_reads(lines[0][len("return"):-1])


def expression_reads(expr):
    if ADDRESS.match(expr):
        return False

    return not any(one in expr for one in ("(", "=", "++", "--"))


def declarations():
    """Every member declared in a header, as (class, name) -> the declaration line."""
    found = {}

    for path, shown in sources("*.h"):
        text = open(path, encoding="utf-8", errors="replace").read()
        klass = None

        for line in text.splitlines():
            hit = re.match(r"^(?:class|struct)\s+(\w+)", line)

            if hit:
                klass = hit.group(1)
                continue

            if klass is None:
                continue

            name = re.search(r"\b(\w+)\s*\([^)]*\)", line)

            if name:
                found.setdefault((klass, name.group(1)), []).append((shown, line))

    return found


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv
    apply = None

    if "--apply" in sys.argv:
        apply = sys.argv[sys.argv.index("--apply") + 1]

    declared = declarations()
    open_ones = collections.defaultdict(list)

    for path, shown in sources("*.cpp"):
        text = open(path, encoding="utf-8", errors="replace").read()

        for hit in OUT_OF_LINE.finditer(text):
            if "const" in hit.group("tail"):
                continue

            if not reads_only(hit.group("body")):
                continue

            key = (hit.group("cls"), hit.group("fn"))
            rows = declared.get(key, [])

            if not rows:
                continue

            if any("virtual" in line or "static" in line or ") const" in line
                   for where, line in rows):
                continue

            open_ones[shown].append((key, rows[0]))

    # The same accessor written inside the class, where declaration and definition are one line.
    for path, shown in sources("*.h"):
        text = open(path, encoding="utf-8", errors="replace").read()

        for hit in INLINE.finditer(text):
            head = hit.group("head")

            if ") const" in head or "static" in head or "virtual" in head:
                continue

            if not expression_reads(hit.group("expr")):
                continue

            open_ones[shown].append(((None, hit.group("fn")), (shown, hit.group(0))))

    total = sum(len(v) for v in open_ones.values())

    if show:
        for shown in sorted(open_ones):
            for (cls, fn), (where, line) in open_ones[shown]:
                print("%-44s %s::%s   declared %s" % (shown, cls, fn, where))

    if apply:
        wrote = write_one(apply, open_ones, declared)
        print("%d change(s) in %s" % (wrote, apply))
        return 0

    print("%d member(s) that only return a member and are not const, over %d file(s)"
          % (total, len(open_ones)))

    return 1 if total else 0


def write_one(target, open_ones, declared):
    """Put const on every candidate whose definition is in `target`, and on its declaration."""
    rows = open_ones.get(target, [])

    if not rows:
        return 0

    text = open(target, encoding="utf-8", newline="").read()
    wrote = 0

    for (cls, fn), (where, line) in rows:
        if cls is None:
            # The inline form: the one line is both the declaration and the definition.
            if text.count(line) != 1:
                raise SystemExit("%d match(es) for the inline %s" % (text.count(line), fn))

            at = line.index(")", line.index("(")) + 1
            text = text.replace(line, line[:at] + " const" + line[at:], 1)
            wrote += 1
            continue

        hits = [hit for hit in OUT_OF_LINE.finditer(text)
                if hit.group("cls") == cls and hit.group("fn") == fn]

        if len(hits) != 1:
            continue

        hit = hits[0]
        text = text[:hit.end("head")] + " const" + text[hit.end("head"):]
        wrote += 1

        head = open(where, encoding="utf-8", newline="").read()
        want = line.rstrip()

        if head.count(want) != 1:
            raise SystemExit("%d match(es) for the declaration of %s::%s" % (head.count(want), cls, fn))

        open(where, "w", encoding="utf-8", newline="").write(
            head.replace(want, want.replace(");", ") const;"), 1))

    open(target, "w", encoding="utf-8", newline="").write(text)

    return wrote


if __name__ == "__main__":
    sys.exit(main())
