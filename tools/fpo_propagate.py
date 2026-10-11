"""Grow the v1.0.0.0 to v1.0.0.1 anchor set along the call graph.

`fpo_anchors.py` pairs functions that reference the same string literal, and few functions
reference one. This grows that set along the call graph: both images come from the same source,
so every paired function is a witness about its unpaired neighbours.

    seed      the literal anchors from --anchors
    vote      each paired function votes for pairings between its unpaired callers on each
              side, and again over its callees. A witness with one candidate each way casts a
              whole vote; one with several splits its vote
    settle    a candidate is taken when it strictly beats the runner-up, strongest first, and
              each far-side function takes one partner
    repeat    until a pass adds nothing

A candidate is rejected when the two sides call different imported DLL symbols.

Calls in v1.0.0.0 go through incremental-link thunks, which are resolved first (see `thunks`).

Size is held out. The matcher never reads it, so the report ends with a size check: the share of
pairs whose sizes agree within 25 per cent. Random pairs rarely do.

Writes a CSV of the propagated pairs to -o, or to stdout.

    python tools/fpo_propagate.py <v1.0.0.0.exe> <v1.0.0.1.exe> [-o symbols/v1000_propagated.csv]
"""

import argparse
import bisect
import collections
import csv
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from fpo_table import fpo_records
from peinfo import PEFile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# A winning candidate needs at least MIN_EVIDENCE of a vote and must beat the runner-up
# by MARGIN. One whole vote is one witness that had a single candidate on each side.
#
# The size check holds across a wide range of evidence floors. The margin must be above zero:
# accepting ties doubles the yield and breaks the size check.
MIN_EVIDENCE = 0.0
MARGIN = 1e-6

# What the held-out size check calls agreement.
SIZE_TOLERANCE = 0.25


class Ranges:
    def __init__(self, spans):
        self.spans = sorted(spans)
        self.starts = [span[0] for span in self.spans]

    def find(self, address):
        index = bisect.bisect_right(self.starts, address) - 1
        if index < 0:
            return None
        start, size = self.spans[index]
        if address < start + size:
            return start
        return None


def text_of(pe):
    section = next(s for s in pe.sections if s["name"] == ".text")
    base = pe.image_base + section["vaddr"]
    blob = pe.data[section["rawoff"]:section["rawoff"] + section["rawsize"]]
    return base, blob


def import_names(pe):
    """Return {import address table slot: 'DLL!symbol'}."""
    directory = pe.optional + 96 + 8
    rva, size = struct.unpack_from("<II", pe.data, directory)
    if not rva or not size:
        return {}

    def offset_of(address):
        for section in pe.sections:
            if section["vaddr"] <= address < section["vaddr"] + max(section["vsize"],
                                                                    section["rawsize"]):
                return section["rawoff"] + (address - section["vaddr"])
        return None

    out = {}
    entry = offset_of(rva)
    if entry is None:
        return {}
    while True:
        lookup, _, _, name_rva, first = struct.unpack_from("<IIIII", pe.data, entry)
        if not name_rva and not first:
            break
        at = offset_of(name_rva)
        dll = pe.data[at:pe.data.index(b"\0", at)].decode("latin1") if at else "?"
        table = offset_of(lookup or first)
        slot = pe.image_base + first
        while table is not None:
            value = struct.unpack_from("<I", pe.data, table)[0]
            if not value:
                break
            if value & 0x80000000:
                out[slot] = "%s!#%d" % (dll, value & 0xFFFF)
            else:
                text_at = offset_of(value) + 2
                end = pe.data.index(b"\0", text_at)
                out[slot] = "%s!%s" % (dll, pe.data[text_at:end].decode("latin1"))
            table += 4
            slot += 4
        entry += 20
    return out


def thunks(pe):
    """Return {thunk address: what it jumps to} for the incremental linker's jump table.

    v1.0.0.0 was linked incrementally. Its .text opens with a table of five-byte `jmp rel32`
    thunks, and most direct calls target a thunk and not the function. v1.0.0.1 has none.
    """
    base, blob = text_of(pe)
    out = {}
    offset = 0
    while offset + 5 <= len(blob):
        if blob[offset] != 0xE9:
            break
        site = base + offset
        out[site] = (site + 5 + struct.unpack_from("<i", blob, offset + 1)[0]) & 0xFFFFFFFF
        offset += 5
    return out


def resolve(address, table, hops=3):
    for _ in range(hops):
        if address not in table:
            return address
        address = table[address]
    return address


def widen(pe, spans, table):
    """Add every direct call target that no span covers, as a span of its own.

    An FPO record exists only where the compiler omitted the frame pointer, so most call
    targets have none. Without these nodes neither call graph can be walked. A node added here
    still pairs; it carries no FPO data.
    """
    base, blob = text_of(pe)
    covered = Ranges(spans)
    starts = set(start for start, _ in spans)
    extra = set()
    offset = 0
    while offset < len(blob) - 5:
        if blob[offset] == 0xE8:
            site = base + offset
            target = (site + 5 + struct.unpack_from("<i", blob, offset + 1)[0]) & 0xFFFFFFFF
            target = resolve(target, table)
            if base <= target < base + len(blob) and target not in starts:
                if covered.find(target) is None:
                    extra.add(target)
            offset += 5
            continue
        offset += 1

    ordered = sorted(starts | extra)
    limit = base + len(blob)
    sized = dict(spans)
    out = []
    for index, start in enumerate(ordered):
        end = ordered[index + 1] if index + 1 < len(ordered) else limit
        out.append((start, sized.get(start, end - start)))
    return out


def graph(pe, spans, imports, table):
    """Return (callees, callers, import use) keyed by function start."""
    base, blob = text_of(pe)
    ranges = Ranges(spans)
    starts = set(start for start, _ in spans)
    callees = collections.defaultdict(set)
    callers = collections.defaultdict(set)
    used = collections.defaultdict(set)

    offset = 0
    limit = len(blob) - 6
    while offset < limit:
        byte = blob[offset]
        if byte == 0xE8:
            site = base + offset
            target = resolve((site + 5 + struct.unpack_from("<i", blob, offset + 1)[0])
                             & 0xFFFFFFFF, table)
            if target in starts:
                owner = ranges.find(site)
                if owner is not None:
                    callees[owner].add(target)
                    callers[target].add(owner)
                offset += 5
                continue
        elif byte == 0xFF and blob[offset + 1] == 0x15:
            slot = struct.unpack_from("<I", blob, offset + 2)[0]
            name = imports.get(slot)
            if name is not None:
                owner = ranges.find(base + offset)
                if owner is not None:
                    used[owner].add(name)
                offset += 6
                continue
        offset += 1
    return callees, callers, used


def propagate(old, new, pairs, evidence=MIN_EVIDENCE, margin=MARGIN):
    """Extend `pairs` (old start to new start) to a fixed point. Returns the passes."""
    old_callees, old_callers, old_imports = old
    new_callees, new_callers, new_imports = new

    old_by_callee = collections.defaultdict(set)
    for function, targets in old_callees.items():
        for target in targets:
            old_by_callee[target].add(function)
    new_by_callee = collections.defaultdict(set)
    for function, targets in new_callees.items():
        for target in targets:
            new_by_callee[target].add(function)

    passes = []
    taken = dict(pairs)
    while True:
        used = set(taken.values())
        votes = collections.defaultdict(collections.Counter)

        for source, target in taken.items():
            for old_edge, new_edge in ((old_by_callee, new_by_callee),
                                       (old_callees, new_callees)):
                left = [f for f in old_edge.get(source, ()) if f not in taken]
                right = [f for f in new_edge.get(target, ()) if f not in used]
                if not left or not right:
                    continue
                weight = 1.0 / (len(left) * len(right))
                for candidate in left:
                    for other in right:
                        votes[candidate][other] += weight

        proposals = []
        for candidate, counter in votes.items():
            ranked = counter.most_common()
            best, score = ranked[0]
            runner_up = ranked[1][1] if len(ranked) > 1 else 0.0
            if score < evidence or score - runner_up < margin:
                continue
            if old_imports.get(candidate, set()) != new_imports.get(best, set()):
                continue
            proposals.append((score, candidate, best))

        # One new function can only be one old function's partner, so the strongest
        # proposal for it wins and the rest wait for the next pass.
        proposals.sort(reverse=True)
        added = []
        claimed = set()
        for score, candidate, best in proposals:
            if best in claimed or best in used or candidate in taken:
                continue
            claimed.add(best)
            taken[candidate] = best
            added.append((candidate, best, "votes", score))

        if not added:
            return passes
        passes.append(added)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("old", help="path to HYPERX.EXE v1.0.0.0")
    parser.add_argument("new", help="path to HYPERX.EXE v1.0.0.1")
    parser.add_argument("-a", "--anchors", default=os.path.join(ROOT, "symbols",
                                                                "v1000_anchors.csv"))
    parser.add_argument("-f", "--functions", default=os.path.join(ROOT, "symbols",
                                                                  "functions.csv"))
    parser.add_argument("-o", "--output", help="write the enlarged set here")
    parser.add_argument("--evidence", type=float, default=MIN_EVIDENCE,
                        help="least vote a winning candidate may have")
    parser.add_argument("--margin", type=float, default=MARGIN,
                        help="how far a winner must beat the runner-up")
    args = parser.parse_args()

    for path in (args.old, args.new, args.anchors, args.functions):
        if not os.path.isfile(path):
            sys.exit("fpo_propagate: no such file: %s" % path)

    old_pe = PEFile(args.old)
    new_pe = PEFile(args.new)

    records = fpo_records(old_pe)
    if not records:
        sys.exit("fpo_propagate: %s has no FPO debug entry" % args.old)

    old_spans = [(old_pe.image_base + r[0], r[1]) for r in records]
    old_size = dict(old_spans)

    new_spans = []
    new_name = {}
    new_size = {}
    with open(args.functions, encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            if not str(row["size"]).isdigit():
                continue
            address = int(row["address"], 16)
            new_spans.append((address, int(row["size"])))
            new_name[address] = row["name"]
            new_size[address] = int(row["size"])

    old_thunks = thunks(old_pe)
    new_thunks = thunks(new_pe)
    old_nodes = widen(old_pe, old_spans, old_thunks)
    new_nodes = widen(new_pe, new_spans, new_thunks)
    old_graph = graph(old_pe, old_nodes, import_names(old_pe), old_thunks)
    new_graph = graph(new_pe, new_nodes, import_names(new_pe), new_thunks)
    sys.stderr.write("fpo_propagate: %d incremental-link thunks in v1.0.0.0, %d in v1.0.0.1\n"
                     % (len(old_thunks), len(new_thunks)))
    sys.stderr.write("fpo_propagate: %d v1.0.0.0 nodes (%d with an FPO record), "
                     "%d v1.0.0.1 nodes (%d named)\n"
                     % (len(old_nodes), len(old_spans), len(new_nodes),
                        len(new_spans)))

    seeds = {}
    with open(args.anchors, encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            seeds[int(row["v1000_address"], 16)] = int(row["v1001_address"], 16)

    passes = propagate(old_graph, new_graph, seeds, args.evidence, args.margin)

    found = dict(seeds)
    rows = []
    for index, added in enumerate(passes):
        for source, target, direction, evidence in added:
            found[source] = target
            rows.append((source, target, index + 1, direction, evidence))

    sys.stderr.write("fpo_propagate: %d seeds, %d passes, %d propagated, %d anchors total "
                     "(%.1f%% of %d FPO functions)\n"
                     % (len(seeds), len(passes), len(rows), len(found),
                        100.0 * len(found) / len(records), len(records)))
    for index, added in enumerate(passes):
        by_direction = collections.Counter(entry[2] for entry in added)
        sys.stderr.write("  pass %d: %d pairs (%s)\n"
                         % (index + 1, len(added),
                            ", ".join("%s %d" % item for item in sorted(by_direction.items()))))

    # The held-out check. Size never entered the matcher, on either the literal seeds or
    # the propagated pairs, so it can still say whether the pairs are real.
    def agreement(pairs):
        close = 0
        total = 0
        for source, target in pairs:
            if source not in old_size or target not in new_size:
                continue
            total += 1
            if abs(old_size[source] - new_size[target]) <= SIZE_TOLERANCE * new_size[target]:
                close += 1
        return close, total

    seed_close, seed_total = agreement(seeds.items())
    grown_close, grown_total = agreement([(r[0], r[1]) for r in rows])
    sys.stderr.write("fpo_propagate: held-out size check, %d of %d seeds within %d%% "
                     "(%.1f%%), %d of %d propagated (%.1f%%)\n"
                     % (seed_close, seed_total, int(SIZE_TOLERANCE * 100),
                        100.0 * seed_close / max(seed_total, 1),
                        grown_close, grown_total,
                        100.0 * grown_close / max(grown_total, 1)))

    lines = ["v1000_address,v1001_address,v1001_name,pass,direction,evidence"]
    for source, target, index, direction, evidence in sorted(rows):
        lines.append("0x%08X,0x%08X,%s,%d,%s,%d"
                     % (source, target, new_name.get(target, "?"), index, direction, evidence))
    body = "\n".join(lines) + "\n"

    if args.output:
        with open(args.output, "w", newline="\n") as handle:
            handle.write(body)
    else:
        sys.stdout.write(body)


if __name__ == "__main__":
    main()
