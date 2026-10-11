"""Reports everything already written down about an address, before you open it.

Rejected spellings are recorded in several places, and this searches all of them:
`docs/re_corrections.md`, `symbols/retired.csv` and the source files under `src/`.

    python tools/prior_work.py 0x0040e0b0
    python tools/prior_work.py 0x0040e0b0 CSteerToTarget
"""
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONTEXT = 1


def norm(address):
    """Every spelling an address gets written in around here."""
    value = int(address, 16)
    return {"0x%08x" % value, "0x%06x" % value, "0x%x" % value,
            "%08x" % value, "%06x" % value}


def hits(path, needles):
    try:
        lines = open(path, encoding="utf-8", errors="replace").read().split("\n")
    except OSError:
        return []
    out = []
    for n, line in enumerate(lines):
        low = line.lower()
        if any(needle in low for needle in needles):
            start = max(0, n - CONTEXT)
            out.append((n + 1, lines[start:n + CONTEXT + 1]))
    return out


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    address = sys.argv[1]
    needles = {s.lower() for s in norm(address)}
    for extra in sys.argv[2:]:
        needles.add(extra.lower())

    targets = []
    targets += [os.path.join(ROOT, "docs", "re_corrections.md"),
                os.path.join(ROOT, "symbols", "retired.csv")]
    for root, _, files in os.walk(os.path.join(ROOT, "src")):
        for name in files:
            if name.endswith((".c", ".cpp", ".h")):
                targets.append(os.path.join(root, name))

    total = 0
    for path in targets:
        found = hits(path, needles)
        if not found:
            continue
        rel = os.path.relpath(path, ROOT)
        for line_no, block in found:
            total += 1
            print("%s:%d" % (rel, line_no))
            for line in block:
                print("    %s" % line.rstrip())
            print()

    if not total:
        print("Nothing recorded for %s. It is genuinely unopened." % address)
    else:
        print("%d mention(s). Read them before spending a spelling." % total)
    return 0


if __name__ == "__main__":
    sys.exit(main())
