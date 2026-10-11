"""Measure how far each annotated global sits from the address the client puts it at.

reccmp resolves a direct `[base]` reference by name, so a misplaced global costs nothing there.
A scaled index, `[reg*4 + base]`, is compared as a literal address, so a table out of place
costs every function that indexes it.

This reads every `// GLOBAL: HYPERX 0xADDR` marker, looks the symbol up in the measured build's
map file, and prints the ones that disagree, worst first. A global lands where the section and
the declaration order put it, so moving one means changing what is declared in front of it.

    python tools/global_drift.py                # every global that is not where it should be
    python tools/global_drift.py --indexed      # only the ones a scaled index reaches
    python tools/global_drift.py --limit 30     # rows to print

The `--indexed` list is the one worth working on.
"""
import argparse
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

MAP = os.path.join(ROOT, "build", "out", "HYPERX_sym.map")
REPORT = os.path.join(ROOT, "build", "report.json")

MARKER = re.compile(r"//\s*GLOBAL:\s*HYPERX\s+0x([0-9a-fA-F]+)")
DECL = re.compile(r"\b(\w+)\s*(?:\[|=|;|\))")

# `0001:0000f2a0  _g_name  004102a0  f  file.obj`
MAP_ROW = re.compile(r"^\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s", re.I)

# A scaled index in an operand, which is the form reccmp cannot resolve by name.
SCALED = re.compile(r"\[\w+\*[1248]\s*\+\s*(0x[0-9a-f]+)\]", re.I)


def read_map():
    if not os.path.exists(MAP):
        sys.exit("no map file; build first")

    out = {}

    for line in open(MAP, encoding="latin-1"):
        found = MAP_ROW.match(line)

        if not found:
            continue

        # Two spellings in one file. C linkage gives `_name`; a C++ object gives the mangled
        # `?name@@3PA...`, whose identifier is the run between the leading question mark and
        # the first double at-sign.
        raw = found.group(1)

        if raw.startswith("?"):
            end = raw.find("@@")
            plain = raw[1:end] if end > 0 else raw[1:]
        else:
            plain = raw.lstrip("_")

        out.setdefault(plain, int(found.group(2), 16))

    return out


def read_markers():
    """Every GLOBAL marker and the name of the declaration it binds to."""
    found = {}

    for where, _, names in os.walk(os.path.join(ROOT, "src")):
        for name in names:
            if not name.endswith((".cpp", ".h")):
                continue

            path = os.path.join(where, name)
            lines = open(path, encoding="utf-8", errors="ignore").read().splitlines()

            for index, line in enumerate(lines):
                hit = MARKER.search(line)

                if not hit:
                    continue

                for ahead in lines[index + 1:index + 10]:
                    stripped = ahead.strip()

                    if not stripped or stripped.startswith("//"):
                        continue

                    # The declared name is the last identifier before a bracket, an equals or
                    # the semicolon; a pointer star and the type sit in front of it.
                    names_on_line = re.findall(r"\b([A-Za-z_]\w*)\b", stripped)
                    picked = None

                    for token in names_on_line:
                        if token.startswith("g_") or token.startswith("s_"):
                            picked = token
                            break

                    if picked is None and names_on_line:
                        picked = names_on_line[-1]

                    if picked:
                        found[picked] = (int(hit.group(1), 16),
                                         os.path.relpath(path, ROOT))

                    break

    return found


def indexed_addresses():
    """Addresses the original reaches through a scaled index, from the report's own diffs."""
    if not os.path.exists(REPORT):
        return set()

    data = json.load(open(REPORT))["data"]
    out = set()

    for row in data:
        for entry in row.get("diff") or []:
            if not isinstance(entry, list) or len(entry) < 2:
                continue

            for grp in entry[1]:
                if not isinstance(grp, dict):
                    continue

                for key in ("both", "orig", "recomp"):
                    for item in grp.get(key, []):
                        for hit in SCALED.finditer(item[1]):
                            out.add(int(hit.group(1), 16))

    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--indexed", action="store_true",
                        help="only globals a scaled index reaches")
    parser.add_argument("--limit", type=int, default=30)
    args = parser.parse_args()

    built = read_map()
    marked = read_markers()
    scaled = indexed_addresses() if args.indexed else None

    rows = []
    missing = 0

    for name, (want, where) in marked.items():
        got = built.get(name)

        if got is None:
            missing += 1
            continue

        if got == want:
            continue

        if scaled is not None and want not in scaled:
            continue

        rows.append((abs(got - want), want, got, name, where))

    rows.sort(reverse=True)

    print("%d markers, %d not in the map, %d adrift%s"
          % (len(marked), missing, len(rows),
             " and reached by a scaled index" if scaled is not None else ""))
    print("%-10s %-10s %-9s %-34s %s" % ("want", "got", "drift", "name", "file"))

    for drift, want, got, name, where in rows[:args.limit]:
        print("0x%08x 0x%08x %+9d %-34s %s" % (want, got, got - want, name[:34], where))

    return 0


if __name__ == "__main__":
    sys.exit(main())
