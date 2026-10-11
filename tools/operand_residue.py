"""Find the rows whose residue is operand naming, not instruction bytes.

Each orig/recomp group of equal length in a row's diff is paired line by line. A pair whose
lines agree once every symbol and image address is blanked is an operand pair: the bytes match
and only the name reccmp gave an operand differs. Everything else is a code line. Operand pairs
are sorted by cause, since each cause has its own fix in the annotations:

  string     one side names a string literal and the other a bare address: a missing STRING
             marker, or a literal our PDB does not name.
  unresolved one side shows `<OFFSETn>`: that address has no annotation or no PDB symbol.
  interior   one side reads symbol+N: an address inside an object, unpairable in a /FIXED image.
  named      one side has a name and the other a bare address: a missing GLOBAL, VTABLE or
             LIBRARY marker, a function marker, or a symbol our PDB does not carry.
  bare       both sides are bare addresses: neither side has a symbol for the datum, such as a
             function-local static or a one-past-the-end bound.
  rival      names on both sides that disagree: folding, or one address marked twice.

It reads build/report.json and costs no build.

    python tools/operand_residue.py               # every row with an operand pair, by share
    python tools/operand_residue.py --only        # rows whose residue is wholly operand pairs
    python tools/operand_residue.py 0x0042d8f0    # one row's operand pairs
"""
import argparse
import collections
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import report

SOURCE = re.compile(r"\s*\t\([^)]*\)\s*$")
STRING = re.compile(r'".*" \(STRING\)')
TAG = re.compile(r" \((?:DATA|OFFSET|FUNCTION|POINTER|FLOAT|VTABLE|STRING)\)")
OFFSET = re.compile(r"<OFFSET\d+>")
INTERIOR = re.compile(r"\S\+\d+ \(OFFSET\)")
ADDR = re.compile(r"^0x0*4[0-9a-f]{5}$")
PLAIN = re.compile(r"^(?:e?(?:ax|bx|cx|dx|si|di|bp|sp)|[abcd][lh]|st\(\d\)|byte|word|dword"
                   r"|qword|tbyte|ptr|offset|short|-?0x[0-9a-f]+|\d+)$")
LOOSE = re.compile(r"S\+\d+")
SPLIT = re.compile(r"\s*(?:[,\[\]*+]|\s-\s|\s)\s*")


def operands(line):
    """The mnemonic, the operand skeleton, and the symbols and bare addresses it holds."""
    text = SOURCE.sub("", line).strip()
    strings = STRING.findall(text)
    text = STRING.sub("S", text)
    text = OFFSET.sub("<O>", text)
    text = TAG.sub("", text)
    head, _, rest = text.partition(" ")
    names = list(strings)
    addresses = []
    skeleton = rest

    for token in SPLIT.split(rest):
        if not token or token == "S":
            continue

        if ADDR.match(token):
            addresses.append(token)
        elif not PLAIN.match(token):
            names.append(token)
        else:
            continue

        skeleton = skeleton.replace(token, "S", 1)

    return head, skeleton, names, addresses


def same(a, b):
    """Skeletons agree, or differ only in a +N after names that differ: the same address."""
    if a[1] == b[1]:
        return True

    return a[2] != b[2] and LOOSE.sub("S", a[1]) == LOOSE.sub("S", b[1])


def cause(orig, ours):
    for pattern, kind in ((STRING, "string"), (OFFSET, "unresolved"), (INTERIOR, "interior")):
        if pattern.search(orig) or pattern.search(ours):
            return kind

    named = [bool(operands(side)[2]) for side in (orig, ours)]

    if named[0] != named[1]:
        return "named"

    if not named[0]:
        return "bare"

    return "rival"


def pairs(row):
    """Every operand pair in the row, and how many code lines are left over."""
    found = []
    code = 0

    for hunk in row.get("diff") or []:
        for group in hunk[1]:
            if "both" in group:
                continue

            origs = [item[1] for item in group.get("orig", [])]
            ours = [item[1] for item in group.get("recomp", [])]

            if len(origs) != len(ours):
                code += max(len(origs), len(ours))
                continue

            for orig, mine in zip(origs, ours):
                a = operands(orig)
                b = operands(mine)

                if a[0] == b[0] and same(a, b) and (a[2] or a[3] or b[2] or b[3]):
                    found.append((cause(orig, mine), SOURCE.sub("", orig).strip(),
                                  SOURCE.sub("", mine).strip()))
                else:
                    code += 1

    return found, code


def float_faults():
    """Operand pairs where one side reads a float constant and the other a placeholder.

    The original side reads (FLOAT) only through a row in symbols/float_constants.csv. Against a
    placeholder that row is missing, or names a library constant our PDB has no symbol for, so
    the row turns a pair of placeholders into a difference.
    """
    out = []

    for row in rows():
        for hunk in row.get("diff") or []:
            for group in hunk[1]:
                origs = [item for item in group.get("orig", [])]
                ours = [item for item in group.get("recomp", [])]

                if "both" in group or len(origs) != len(ours):
                    continue

                for orig, mine in zip(origs, ours):
                    floats = ["(FLOAT)" in orig[1], "(FLOAT)" in mine[1]]

                    if floats[0] != floats[1] and "<OFFSET" in orig[1] + mine[1]:
                        out.append((row["name"], orig[0], orig[1], mine[1]))

    return out


def rows():
    for row in report.load():
        if row.get("matching", 1.0) < 1.0 and row.get("diff"):
            yield row


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("address", nargs="?")
    parser.add_argument("--only", action="store_true", help="rows with no code line left")
    args = parser.parse_args()

    if args.address:
        want = int(args.address, 16)

        for row in rows():
            if int(row["address"], 16) == want:
                found, code = pairs(row)
                print("%s  %.2f%%  %d operand pairs, %d code lines"
                      % (row["name"], row["matching"] * 100, len(found), code))

                for kind, orig, mine in found:
                    print("  %-10s %s\n  %-10s %s" % (kind, orig, "", mine))

        return

    totals = collections.Counter()
    listed = []

    for row in rows():
        found, code = pairs(row)

        if not found or (args.only and code):
            continue

        kinds = collections.Counter(kind for kind, _, _ in found)
        totals.update(kinds)
        listed.append((code / (code + len(found)), -len(found), row, kinds, code))

    listed.sort(key=lambda entry: entry[:2])

    for share, _, row, kinds, code in listed:
        print("%-9s %6.2f%%  %3d code  %-42s %s" % (
            row["address"], row["matching"] * 100, code, row["name"][:42],
            " ".join("%s=%d" % item for item in sorted(kinds.items()))))

    print("%d rows with operand pairs, %d wholly operand: %s" % (
        len(listed), sum(1 for entry in listed if entry[4] == 0),
        " ".join("%s=%d" % item for item in sorted(totals.items()))))


if __name__ == "__main__":
    main()
