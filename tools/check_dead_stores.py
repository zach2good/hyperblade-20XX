"""Fail on two consecutive writes to one name where the first cannot be read.

Two writes in a row to one slot, where the first is never read, usually mean the decompiled
blocks were folded together and one write belongs elsewhere. For example:

    ppOnDrome = m_pTeamA->apOnIce;
    ppOnDrome = m_pTeamB->apOnIce;

Accepted pairs are listed in symbols/dead_stores.csv, each with its reason. Typical ones are a
result captured after each of two calls, and a store the client makes twice.

The left hand side must be a plain name or a member chain off one: `p`, `pIt->m_nField`, `a.b.c`.
Stores through a dereference or a cast, such as `*p = 1`, are not matched, because in this tree
those are mostly deliberate byte spellings.

    python tools/check_dead_stores.py
    python tools/check_dead_stores.py --all    every pair, accepted ones included

Exit code 1 when an unregistered pair is found, or when a registered one has gone.
"""
import csv
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")
REGISTER = os.path.join(ROOT, "symbols", "dead_stores.csv")

# A plain assignment: one name, possibly through members, then a single `=`.
ASSIGN = re.compile(r"^(\s*)([A-Za-z_][A-Za-z0-9_]*(?:(?:->|\.)[A-Za-z0-9_]+)*)\s*=\s*([^=].*);\s*$")


def sourceNames(root):
    # Paths come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def accepted():
    rows = {}

    if not os.path.exists(REGISTER):
        return rows

    with open(REGISTER, encoding="utf-8") as handle:
        for row in csv.DictReader(line for line in handle if not line.startswith("#")):
            rows[(row["file"].replace("\\", "/"), row["name"])] = row.get("reason", "")

    return rows


def pairs(path):
    """Consecutive writes to one name, at one indent, where the second does not read it."""
    text = open(path, encoding="latin1").read()
    found = []
    previous = None

    for number, line in enumerate(text.splitlines(), 1):
        stripped = line.strip()

        # A blank line or a comment between the two writes does not break the run.
        if not stripped or stripped.startswith("//"):
            continue

        match = ASSIGN.match(line)

        if not match:
            previous = None
            continue

        indent, name, value = match.group(1), match.group(2), match.group(3)
        root = name.split("->")[0].split(".")[0]

        if (previous and previous[0] == indent and previous[1] == name
                and not re.search(r"\b%s\b" % re.escape(root), value)):
            found.append((previous[2], number, name))

        previous = (indent, name, number)

    return found


def main():
    show = "--all" in sys.argv
    keep = accepted()
    kept = set()
    faults = []
    counted = 0

    for name in sorted(sourceNames(SRC)):
        if not name.endswith(".cpp"):
            continue

        for first, second, written in pairs(os.path.join(SRC, name)):
            counted += 1
            key = ("src/" + name, written)

            if key in keep:
                kept.add(key)

                if show:
                    print("ok   %s:%d-%d  %s" % ("src/" + name, first, second, written))
            else:
                faults.append(("src/" + name, first, second, written))

    for path, first, second, written in faults:
        print("%s:%d-%d  %s is written twice and the first cannot be read"
              % (path, first, second, written))

    stale = sorted(set(keep) - kept)

    for path, written in stale:
        print("STALE  %s keeps %s in %s and the pair has gone" % (REGISTER, written, path))

    print("%d consecutive write pair(s), %d accepted, %d unregistered"
          % (counted, len(kept), len(faults)))

    if faults:
        print("A dead write is a transcription fault until a measurement says otherwise.")

    return 1 if faults or stale else 0


if __name__ == "__main__":
    sys.exit(main())
