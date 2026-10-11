"""Anchor v1.0.0.0 functions to their named v1.0.0.1 counterparts through string literals.

fpo_table.py reads exact function boundaries out of v1.0.0.0's debug directory, but a
v1.0.0.0 address is worthless here on its own: the two images were built by different
compilers and share no addresses.

String literals survive the recompile. A literal that exactly one function references in
each image identifies that pair of functions, whatever the compiler did to the code
around it. This walks both images, attributes every literal reference to its containing
function, and emits the pairs where both sides are unambiguous.

What an anchor buys you: a name for the v1.0.0.0 function, and its exact size and
parameter dword count from the FPO record, to check a v1.0.0.1 decode against. A
signature that disagrees on parameter count is wrong on one of the two sides.

Usage:
    python tools/fpo_anchors.py <v1.0.0.0.exe> <v1.0.0.1.exe> [-o symbols/v1000_anchors.csv]
"""

import argparse
import bisect
import collections
import csv
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from fpo_table import fpo_records
from peinfo import PEFile

MIN_LITERAL = 6

# A winning pair must beat the runner-up by at least one whole literal.
MARGIN = 1.0
LITERAL = re.compile(rb"[\x20-\x7e]{%d,}\x00" % MIN_LITERAL)


def literals(pe):
    """Return {virtual address: text} for every NUL-terminated literal in the data sections."""
    out = {}
    for section in pe.sections:
        if section["name"] not in (".rdata", ".data"):
            continue
        blob = pe.data[section["rawoff"]:section["rawoff"] + section["rawsize"]]
        for match in LITERAL.finditer(blob):
            address = pe.image_base + section["vaddr"] + match.start()
            out[address] = match.group()[:-1].decode("latin1")
    return out


def references(pe, wanted):
    """Return [(virtual address of the reference, target), ...] for immediates in .text."""
    section = next(s for s in pe.sections if s["name"] == ".text")
    blob = pe.data[section["rawoff"]:section["rawoff"] + section["rawsize"]]
    base = pe.image_base + section["vaddr"]
    out = []
    for offset in range(len(blob) - 3):
        value = struct.unpack_from("<I", blob, offset)[0]
        if value in wanted:
            out.append((base + offset, value))
    return out


class Ranges:
    """Address to enclosing function lookup over non-overlapping [start, start+size) spans."""

    def __init__(self, spans):
        self.spans = sorted(spans)
        self.starts = [span[0] for span in self.spans]

    def find(self, address):
        index = bisect.bisect_right(self.starts, address) - 1
        if index < 0:
            return None
        start, size, payload = self.spans[index]
        if address < start + size:
            return payload
        return None


def load_v1001_functions(path):
    spans = []
    with open(path, newline="") as handle:
        for row in csv.DictReader(handle):
            spans.append((int(row["address"], 16), int(row["size"]), row["name"]))
    return spans


def attribute(pe, ranges, texts):
    """Return {literal text: set of function keys that reference it}."""
    by_address = {address: text for address, text in texts.items()}
    found = collections.defaultdict(set)
    for site, target in references(pe, by_address):
        owner = ranges.find(site)
        if owner is None:
            continue
        found[by_address[target]].add(owner)
    return found


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("old", help="path to HYPERX.EXE v1.0.0.0")
    parser.add_argument("new", help="path to HYPERX.EXE v1.0.0.1")
    parser.add_argument("-f", "--functions", default="symbols/functions.csv")
    parser.add_argument("-o", "--output", help="write CSV here instead of stdout")
    args = parser.parse_args()

    for path in (args.old, args.new, args.functions):
        if not os.path.isfile(path):
            sys.exit("fpo_anchors: no such file: %s" % path)

    old = PEFile(args.old)
    new = PEFile(args.new)

    old_records = fpo_records(old)
    if not old_records:
        sys.exit("fpo_anchors: %s has no FPO debug entry. The first image must be v1.0.0.0."
                 % args.old)

    old_spans = [(old.image_base + r[0], r[1], (old.image_base + r[0], r[1], r[3]))
                 for r in old_records]
    new_spans = load_v1001_functions(args.functions)

    old_texts = literals(old)
    new_texts = literals(new)
    shared = set(old_texts.values()) & set(new_texts.values())

    old_refs = attribute(old, Ranges(old_spans),
                         {a: t for a, t in old_texts.items() if t in shared})
    new_refs = attribute(new, Ranges([(a, s, (a, s, n)) for a, s, n in new_spans]),
                         {a: t for a, t in new_texts.items() if t in shared})

    # Each literal votes for the pairs it could name, split evenly when several functions on a
    # side share it. A literal used by one function on each side casts a full vote.
    votes = collections.defaultdict(collections.Counter)
    evidence = collections.defaultdict(list)
    for text in shared:
        left = old_refs.get(text, set())
        right = new_refs.get(text, set())
        if not left or not right:
            continue
        weight = 1.0 / (len(left) * len(right))
        for source in left:
            for target in right:
                votes[source][target] += weight
                if weight == 1.0:
                    evidence[(source, target)].append(text)

    rows = []
    contested = 0
    for source, counter in votes.items():
        ranked = counter.most_common()
        runner_up = ranked[1][1] if len(ranked) > 1 else 0.0
        if ranked[0][1] - runner_up < MARGIN:
            contested += 1
            continue
        target, score = ranked[0]
        witnesses = evidence.get((source, target)) or ["<weighted>"]
        sample = sorted(witnesses, key=len, reverse=True)[0]
        rows.append((source[0], source[1], source[2], target[0], target[2], target[1],
                     round(score, 2), len(ranked) - 1, sample))

    rows.sort()
    lines = ["v1000_address,v1000_size,v1000_param_dwords,"
             "v1001_address,v1001_name,v1001_size,score,rivals,witness"]
    for row in rows:
        lines.append("0x%08X,%d,%d,0x%08X,%s,%d,%g,%d,%s"
                     % (row[0], row[1], row[2], row[3], row[4], row[5], row[6], row[7],
                        '"%s"' % row[8].replace('"', '""')))
    body = "\n".join(lines) + "\n"

    if args.output:
        with open(args.output, "w", newline="\n") as handle:
            handle.write(body)
    else:
        sys.stdout.write(body)

    sys.stderr.write("fpo_anchors: %d shared literals, %d anchors, %d contested\n"
                     % (len(shared), len(rows), contested))
    sys.stderr.write("fpo_anchors: %d of %d FPO functions anchored (%.1f%%)\n"
                     % (len(rows), len(old_records), 100.0 * len(rows) / len(old_records)))


if __name__ == "__main__":
    main()
