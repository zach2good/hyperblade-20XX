# Flags a statement that steps pointers by the same literal in both byte and element units.
#
# Usage: python tools/check_mixed_units.py [--list]
#   --list  print each site.
# Exits 1 on any unregistered site. symbols/mixed_units.csv registers accepted (file, name) pairs.
#
# `p + 4` on a `float*` moves sixteen bytes; `(char*)p + 4` moves four. Each spelling is correct
# alone, but a statement that uses `(char*)A + N` beside `B + N`, with B wider than a byte, has
# at most one of them right:
#
#     *(float*)(m_pflSpeed + 4)  = *(float*)((char*)m_pStep + 4)  + ...
#
# Members are included, which check_scaled_offsets.py cannot see. A member name that two headers
# declare with different types is skipped, since it is two fields.

import collections
import os
import re
import sys

import comments

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REGISTER = os.path.join("symbols", "mixed_units.csv")

# `return *m_pfl;` in an inline body has the same shape as a declaration, so keywords are refused.
MEMBER = re.compile(r"^\s*(?:const\s+|volatile\s+|mutable\s+|struct\s+|class\s+)*"
                    r"(?!(?:return|delete|throw)\b)"
                    r"([A-Za-z_]\w*)\s*\*\s*(m_[A-Za-z_]\w*)\s*;", re.M)
LOCAL = re.compile(r"^\s*(?:const\s+|volatile\s+|static\s+|struct\s+|class\s+)*"
                   r"([A-Za-z_]\w*)\s*\*\s*(?:const\s+)?([A-Za-z_]\w*)\s*[;,)=]")

BYTE_STEP = re.compile(r"\(\s*(?:const\s+|volatile\s+)*(?:unsigned\s+|signed\s+)?char\s*\*\s*\)"
                       r"\s*([A-Za-z_]\w*)\s*\+\s*(0x[0-9a-fA-F]+|\d+)\b")
ELEM_STEP = re.compile(r"(?<![>.\w])([A-Za-z_]\w*)\s*\+\s*(0x[0-9a-fA-F]+|\d+)\b")

BYTE_WIDE = ("char", "void")
SIGNATURE = re.compile(r"^[A-Za-z_][\w:<>,\s\*&]*::~?\w+\s*\(|^[A-Za-z_][\w:<>,\s\*&]*\s+\w+\s*\(")


def sourceNames(root):
    found = []

    for directory, unused, names in os.walk(root):
        for name in sorted(names):
            if name.endswith((".cpp", ".h")):
                found.append(os.path.join(directory, name))

    return found


def memberTypes(files):
    """Member name to its declared pointer type, for names only one header declares."""
    seen = collections.defaultdict(set)

    for path in files:
        if not path.endswith(".h"):
            continue

        for kind, name in MEMBER.findall(open(path, encoding="utf-8",
                                                 errors="replace").read()):
            seen[name].add(kind)

    return {name: list(kinds)[0] for name, kinds in seen.items() if len(kinds) == 1}


def registered():
    if not os.path.exists(REGISTER):
        return set()

    import csv

    with open(REGISTER, encoding="utf-8") as handle:
        return {(row["file"], row["name"]) for row in csv.DictReader(handle) if row.get("name")}


def bodies(text):
    """Each function body as (first line number, its lines), split on a column-nought signature."""
    lines = text.split("\n")
    starts = [i for i, line in enumerate(lines) if SIGNATURE.match(line)]
    starts.append(len(lines))

    for a, b in zip(starts, starts[1:]):
        yield a + 1, lines[a:b]


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    files = sourceNames("src")
    members = memberTypes(files)
    known = registered()
    found = []

    for path in files:
        if not path.endswith(".cpp"):
            continue

        shown = os.path.relpath(path, "src").replace(os.sep, "/")
        text = open(path, encoding="utf-8", errors="replace").read()

        for at, lines in bodies(text):
            code = comments.code("\n".join(lines))
            local = {name: kind for kind, name in LOCAL.findall(code)}

            # Per statement: two pointers into parallel data mix only within one expression.
            for statement in code.split(";"):
                byte = BYTE_STEP.findall(statement)

                if not byte:
                    continue

                # The byte step holds an element step inside it: `(char*)p + 4` contains the
                # substring `p + 4`. The byte steps come out before the element scan runs.
                elem = ELEM_STEP.findall(BYTE_STEP.sub(" ", statement))
                shared = {term for unused, term in byte} & {term for unused, term in elem}

                for name, term in sorted(set(elem)):
                    if term not in shared:
                        continue

                    kind = local.get(name) or members.get(name)

                    if not kind or kind in BYTE_WIDE or (shown, name) in known:
                        continue

                    found.append((shown, at, name, kind))

    if show:
        for shown, at, name, kind in found:
            print("MIXED  %-40s %-5d %-22s declared %s*" % (shown, at, name, kind))

    print("%d pointer(s) stepped in both units inside one body, %d registered"
          % (len(found), len(known)))

    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
