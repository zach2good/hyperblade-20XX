"""Sorts the rows that are not exact by the cause of their difference rather than by score.

Four classes:

  FOLDING   The client linked with /OPT:REF and folded identical bodies, so one address names
            several functions. The scored build uses /OPT:NOREF and keeps them apart, so any
            operand naming a folded body reads as the wrong function. No source edit reaches it.

  DATA      An operand carries a literal address the linker chose, and the two builds put that
            datum in different places. Most of it is the C runtime's own data, which cannot move.

  X87       The only difference is where one instruction sits among floating point ones. VC6
            schedules the FPU differently from the client's compiler, and the source has almost
            no control over it.

  open      Everything else, which is where the work is.

    python tools/fault_classes.py                 # the counts, by bytes
    python tools/fault_classes.py --open          # what the open bucket is made of
    python tools/fault_classes.py --list DATA     # every row in one class
    python tools/fault_classes.py --globals       # the globals the DATA class is blocked on

The default view counts bytes short as well as rows, since the walls hold many small rows and
little of the missing bytes. --open splits the open bucket's differences into register naming,
branch displacement and real differences, and ranks rows by real differences.

Reads build/report.json and build/out/HYPERX_sym.map, and runs tools/progress.py for row sizes.
"""
import argparse
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

REPORT = os.path.join(ROOT, "build", "report.json")
MAP = os.path.join(ROOT, "build", "out", "HYPERX_sym.map")

# `   99.64%  0x0041cdd0  BehaviorRegistry::Init                   992 bytes`
PROGRESS_ROW = re.compile(r"\s+([\d.]+)%\s+(0x[0-9a-f]+)\s+(\S+)\s+(\d+) bytes")

# The bodies the client's linker folded together. An operand naming one of these in the client
# and something else here is folding, not a decode error.
FOLD_NAMES = ("Object::Trace", "KeyConfigDialog::SaveKeysAs", "KeyConfigDialog::ClearKeys")

# An address in the image, as an operand rather than as a named symbol.
ADDR = re.compile(r"\b0x0*4[0-9a-f]{5}\b")

FLOAT_OP = re.compile(r"^\s*f[a-z0-9]+\b")

# For --open: a difference that is only which register was picked, or only how far a
# branch reaches.
REG = re.compile(r"\b(?:e?(?:ax|bx|cx|dx|si|di|bp|sp)|[abcd][lh])\b")
DISP = re.compile(r"^(j\w+|call|loop\w*)\s+(-?0x[0-9a-f]+)$")

# `0001:0000f2a0  _g_name  004102a0  f  file.obj`
MAP_ROW = re.compile(r"^\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s", re.I)


def rowSizes():
    """How big each row is, which the report does not say and progress.py prints."""
    import subprocess

    try:
        done = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "progress.py")],
                              capture_output=True, timeout=900, cwd=ROOT)
    except Exception:
        return {}

    out = {}

    for line in done.stdout.decode("latin-1").splitlines():
        found = PROGRESS_ROW.match(line)

        if found:
            out[int(found.group(2), 16)] = int(found.group(4))

    return out


def read_map():
    """Every symbol the scored build placed, by address, so an operand can be named."""
    if not os.path.exists(MAP):
        return {}

    out = {}

    for line in open(MAP, encoding="latin-1"):
        found = MAP_ROW.match(line)

        if not found:
            continue

        raw = found.group(1)

        if raw.startswith("?"):
            end = raw.find("@@")
            plain = raw[1:end] if end > 0 else raw[1:]
        else:
            plain = raw.lstrip("_")

        out.setdefault(int(found.group(2), 16), plain)

    return out


def nearest(symbols, address):
    """The symbol an address falls in, which for a table is the table's own name."""
    best = None

    for at, name in symbols.items():
        if at <= address and (best is None or at > best[0]):
            best = (at, name)

    if best is None or address - best[0] > 0x2000:
        return None

    return best[1]


def classify(row):
    """The class, and the operand pairs that decided it."""
    origs = []
    recos = []
    floats = 0

    for hunk in row.get("diff") or []:
        for group in hunk[1]:
            if "both" in group:
                for item in group["both"]:
                    if FLOAT_OP.match(item[1]):
                        floats += 1

                continue

            for item in group.get("orig", []):
                origs.append(item[1])

            for item in group.get("recomp", []):
                recos.append(item[1])

    text = " ".join(origs + recos)

    if any(name in text for name in FOLD_NAMES):
        return "FOLDING", origs, recos

    if ADDR.search(text) and len(origs) == len(recos):
        return "DATA", origs, recos

    if floats > 0 and len(origs) == len(recos) and len(origs) <= 3:
        return "X87", origs, recos

    return "open", origs, recos


def openShape(rows):
    """Split the open bucket's differences into the two walls and the rest.

    reccmp calls a row effective when every difference is a register name, so those are already
    out of this bucket. Pairs are taken within a diff group; zipping across groups would pair
    unrelated instructions.
    """
    total = 0
    regonly = 0
    disponly = 0
    ranked = []

    for score, name, address, row in rows:
        seen = 0
        walled = 0

        for hunk in row.get("diff") or []:
            for group in hunk[1]:
                if "both" in group:
                    continue

                origs = [i[1] for i in group.get("orig", [])]
                recos = [i[1] for i in group.get("recomp", [])]

                if len(origs) != len(recos):
                    seen += max(len(origs), len(recos))

                    continue

                for a, b in zip(origs, recos):
                    seen += 1

                    if a == b:
                        continue

                    if (REG.sub("R", a.split("\t")[0]).strip()
                            == REG.sub("R", b.split("\t")[0]).strip()):
                        regonly += 1
                        walled += 1

                        continue

                    ma = DISP.match(a.split("\t")[0].strip())
                    mb = DISP.match(b.split("\t")[0].strip())

                    if ma and mb and ma.group(1) == mb.group(1):
                        disponly += 1
                        walled += 1

        total += seen

        if seen:
            ranked.append((seen - walled, seen, score, name, address))

    return total, regonly, disponly, ranked


def retiredAddresses():
    """The addresses symbols/retired.csv lists."""
    path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "symbols",
                        "retired.csv")
    found = set()

    for line in open(path, encoding="utf-8"):
        if line.startswith("0x"):
            found.add(int(line.split(",", 1)[0], 16))

    return found


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--list", help="print every row in one class")
    parser.add_argument("--globals", action="store_true",
                        help="the globals the DATA class is blocked on, worst first")
    parser.add_argument("--open", action="store_true",
                        help="how much of the open bucket is register naming and branches")
    parser.add_argument("--limit", type=int, default=6)
    parser.add_argument("--retired", action="store_true",
                        help="with --open, also rank rows symbols/retired.csv gave up")
    args = parser.parse_args()

    if not os.path.exists(REPORT):
        sys.exit("no report; build first")

    data = json.load(open(REPORT))["data"]
    tally = {}

    for row in data:
        score = row.get("matching")

        if score is None or score >= 1.0 or row.get("effective"):
            continue

        kind, origs, recos = classify(row)
        tally.setdefault(kind, []).append((score, row["name"], row["address"],
                                           origs, recos, row))

    total = sum(len(v) for v in tally.values())

    if args.open:
        total, regonly, disponly, ranked = openShape(
            [(a, b, c, f) for a, b, c, d, e, f in tally.get("open", [])])
        rest = total - regonly - disponly

        print("open bucket, %d difference slots in all" % total)
        print("  register naming only   %6d  (%4.1f%%)" % (regonly, 100.0 * regonly / total))
        print("  branch displacement    %6d  (%4.1f%%)" % (disponly, 100.0 * disponly / total))
        print("  everything else        %6d  (%4.1f%%)" % (rest, 100.0 * rest / total))

        ranked.sort(key=lambda r: -r[0])

        # A retired row was measured and given up, so the ranking leaves it out unless asked.
        if not args.retired:
            given = retiredAddresses()
            hidden = len([r for r in ranked if int(r[4], 16) in given])
            ranked = [r for r in ranked if int(r[4], 16) not in given]
            print("\n%d retired row(s) left out of the ranking; --retired shows them" % hidden)

        print("\nmost differences that are NEITHER, which is where a decode is wrong:")
        print("%-6s %-6s %-8s %s" % ("real", "all", "score", "name"))

        for real, seen, score, name, address in ranked[:args.limit * 2]:
            print("%6d %6d %7.2f%%  %-42s %s" % (real, seen, score * 100, name[:42], address))

        return 0

    if args.globals:
        symbols = read_map()
        blamed = {}

        for score, name, address, origs, recos, row in tally.get("DATA", []):
            for text in recos:
                for hit in ADDR.finditer(text):
                    owner = nearest(symbols, int(hit.group(0), 16))

                    if owner is None:
                        owner = "unnamed %s" % hit.group(0)

                    blamed.setdefault(owner, []).append(name)

        print("%d rows in DATA reach %d distinct data symbols" % (len(tally.get("DATA", [])),
                                                                  len(blamed)))
        print("%-5s %s" % ("rows", "symbol, and the rows it costs"))

        for owner in sorted(blamed, key=lambda k: -len(blamed[k])):
            rows = blamed[owner]
            print("%5d %-34s %s" % (len(rows), owner[:34],
                                    ", ".join(sorted(set(rows))[:3])))

        return 0

    if args.list:
        rows = sorted(tally.get(args.list, []), reverse=True)

        print("%d rows in %s" % (len(rows), args.list))

        for score, name, address, origs, recos, row in rows:
            print("  %6.2f%%  %-46s %s" % (score * 100, name[:46], address))

            for a, b in zip(origs, recos):
                print("            orig %s" % a)
                print("            reco %s" % b)

        return 0

    sizes = rowSizes()
    short = {}

    for kind in tally:
        short[kind] = sum(sizes.get(int(a, 16), 0) * (1.0 - m)
                          for m, n, a, o, r, w in tally[kind])

    spent = sum(short.values())

    print("%d rows are neither exact nor effective, %.0f bytes short of exact\n"
          % (total, spent))

    # Sorted by bytes short, since a vtable row is small and a function row is large.
    for kind in sorted(tally, key=lambda k: -short[k]):
        rows = sorted(tally[kind], reverse=True)

        print("%-9s %4d rows (%4.1f%%)  %7.0f bytes short (%4.1f%%)"
              % (kind, len(rows), 100.0 * len(rows) / total, short[kind],
                 100.0 * short[kind] / spent if spent else 0.0))

        for score, name, address, origs, recos, row in rows[:args.limit]:
            print("      %6.2f%%  %-44s %s" % (score * 100, name[:44], address))

        print("")

    return 0


if __name__ == "__main__":
    sys.exit(main())
