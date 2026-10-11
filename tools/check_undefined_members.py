# Report a member function declared in a header, defined nowhere, with no comment saying why.
#
# A stale declaration can be removed, but removing members can change register allocation in
# code that includes the header, so this asks for a reason rather than for deletion. A comment
# above a run of bodyless declarations covers the whole run. A pure virtual (`= 0`) is skipped.
#
#   python tools/check_undefined_members.py
#   python tools/check_undefined_members.py --list    # also confirm when every one has a reason
#
# Read only. Exit code 1 when a bodyless declaration has no reason above it.

import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SKIP = ("src/jpeg/",)

OPEN = re.compile(r"^\s*(?:class|struct)\s+(\w+)\s*(?::[^;{]*)?\{?\s*$")
# A declaration: an optional return type, the name, the argument list, a semicolon, and
# nothing else. Refusing keywords as names drops `return (T*)(this + 1);` in an inline body and
# a function pointer member such as `void (__stdcall* m_pfn)(void);`.
DECL = re.compile(r"^\s+(?:virtual\s+)?(?:static\s+)?(?:inline\s+)?"
                  r"(?:[\w:]+(?:\s*[*&]+)?\s+)*?(~?\w+)\s*\([^;{)]*\)\s*(?:const\s*)?;\s*$")
DEFN = re.compile(r"\b(\w+)::(~?\w+)\s*\(")
STRUCTURE = re.compile(r"^\s*(?:\{|\}|public:|private:|protected:|"
                       r"(?:class|struct)\s+\w+[^;]*)?\s*$")

KEYWORDS = set("return if else for while switch do sizeof delete new throw case goto "
               "void int char short long float double bool unsigned signed const static "
               "struct class union enum typedef".split())


def sources():
    for suffix in ("*.cpp", "*.h"):
        for path in sorted(glob.glob(os.path.join("src", "**", suffix), recursive=True)):
            shown = path.replace(os.sep, "/")

            if not shown.startswith(SKIP):
                yield shown


def defined():
    """Every Class::Member this tree defines or calls by its qualified name."""
    found = set()

    for shown in sources():
        body = code(open(shown, encoding="utf-8", errors="replace").read())

        for owner, name in DEFN.findall(body):
            found.add((owner, name))

    return found


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    have = defined()
    faults = []
    rows = []
    bodyless = {}
    text = {}
    declared = 0
    reasoned = 0

    for shown in sources():
        if not shown.endswith(".h"):
            continue

        raw = open(shown, encoding="utf-8", errors="replace").read().splitlines()
        lines = code("\n".join(raw)).split("\n")
        text[shown] = raw
        bodyless.setdefault(shown, set())
        cls = None
        depth = 0

        for n, line in enumerate(lines):
            found = OPEN.match(line)

            if found:
                cls = found.group(1)
                depth = line.count("{")
                continue

            if not cls:
                continue

            depth += line.count("{") - line.count("}")

            if depth < 0 or line.startswith("}"):
                cls = None
                continue

            # A member is declared at the class's own depth; deeper is a statement in an inline body.
            if depth != 1:
                continue

            what = DECL.match(line)

            if not what or what.group(1).lstrip("~") in KEYWORDS:
                continue

            declared += 1

            if "= 0" in line or (cls, what.group(1)) in have:
                continue

            bodyless.setdefault(shown, set()).add(n)
            rows.append((shown, n, cls, what.group(1), line.strip()))

    for shown, n, cls, name, line in rows:
        raw = text[shown]

        # Walk up over bodyless siblings and class structure lines, then look for a comment. A
        # note above `class X {` counts for its first member.
        at = n

        while at > 0 and ((at - 1) in bodyless[shown] or STRUCTURE.match(raw[at - 1])):
            at -= 1

        if at > 0 and raw[at - 1].strip().startswith("//"):
            reasoned += 1
            continue

        faults.append((shown, n + 1, cls, name, line))

    for shown, n, cls, name, line in faults:
        print("%-40s %s::%s has no body and no reason above it"
              % ("%s:%d" % (shown, n), cls, name))

    if show and not faults:
        print("%d member declaration(s), %d with no body anywhere and every one with a reason "
              "written above it" % (declared, reasoned))

    print("%d member declaration(s) in headers, %d with no body and no reason above them"
          % (declared, len(faults)))

    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
