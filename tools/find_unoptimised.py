"""List functions the original built without optimisation.

Some translation units in HYPERX.EXE were compiled at /Od. Those functions cannot be
matched from source at /O2 whatever the shape, so they have to be found before they are
written, not after.

The signature, all four of which /O2 removes:

  * a frame pointer on a function that needs no frame
  * arguments copied straight into negative EBP slots and read back from memory
    (optimised code writes spill slots too, but does not keep loading them back)
  * the same comparison against the same slot twice in a row
  * a switch subject mutated in memory rather than in a register

A function is reported when it opens with the frame pointer pair and touches at least two
negative EBP slots. It is marked "strong" when it also reads a local back three times or more.
Unmarked hits include optimised code that keeps a frame pointer, so confirm them with a build.
The filter is loose on purpose: a missed /Od function costs far more than a checked candidate.

    python tools/find_unoptimised.py            every candidate
    python tools/find_unoptimised.py --unwritten  only the ones with no marker yet

Requires the Ghidra bridge that tools/ghidra_api.py names.
"""

import argparse
import csv
import os
import re
import urllib.request
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
from ghidra_api import BASE as BRIDGE  # noqa: E402

MARKER = re.compile(r"//\s*(?:FUNCTION|STUB|LIBRARY):\s*HYPERX\s+(0x[0-9a-fA-F]+)")
EBP_LOCAL = re.compile(r"\[EBP \+ -0x")
# A read of a local, rather than a write to one.
EBP_READ = re.compile(r"^(?:MOV \w+,dword ptr \[EBP \+ -0x|CMP dword ptr \[EBP \+ -0x|MOVSX \w+,word ptr \[EBP \+ -0x)")
CRT_START = 0x494000


def marked_addresses():
    found = set()
    for directory, _, names in os.walk(os.path.join(ROOT, "src")):
        for name in names:
            if not name.endswith((".cpp", ".c", ".h")):
                continue
            path = os.path.join(directory, name)
            with open(path, encoding="utf-8", errors="replace") as handle:
                for line in handle:
                    match = MARKER.search(line)
                    if match:
                        found.add(int(match.group(1), 16))
    return found


def sizes():
    path = os.path.join(ROOT, "symbols", "functions.csv")
    with open(path, encoding="utf-8") as handle:
        return {int(r["address"], 16): int(r["size"]) for r in csv.DictReader(handle)}


def listing(address):
    url = "%s/disassemble_function?address=0x%08x" % (BRIDGE, address)
    try:
        body = urllib.request.urlopen(url, timeout=90).read().decode("utf-8", "replace")
    except Exception:
        return None
    return [line.split(": ", 1)[1].rstrip() for line in body.splitlines() if ": " in line]


def unoptimised(lines):
    if lines is None or len(lines) < 6:
        return False
    if lines[0] != "PUSH EBP" or lines[1] != "MOV EBP,ESP":
        return False
    if sum(1 for line in lines if EBP_LOCAL.search(line)) < 2:
        return False
    return True


def reads_locals_back(lines):
    """The discriminating half, reported but not required.

    Optimised code writes spill slots too. Only unoptimised code keeps loading the value
    out again instead of holding it in a register, so three or more reads is a strong
    candidate and fewer is one to confirm with a build.
    """
    return sum(1 for line in lines if EBP_READ.match(line)) >= 3


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--unwritten", action="store_true")
    args = parser.parse_args()

    url = "%s/list_functions?limit=6000" % BRIDGE
    text = urllib.request.urlopen(url, timeout=120).read().decode("utf-8", "replace")
    functions = []
    for line in text.splitlines():
        match = re.match(r"^(\S+) at ([0-9a-fA-F]{8})\s*$", line.strip())
        if match:
            functions.append((match.group(1), int(match.group(2), 16)))

    known = marked_addresses()
    by_size = sizes()
    wanted = [(n, a) for n, a in functions if a < CRT_START]
    if args.unwritten:
        wanted = [(n, a) for n, a in wanted if a not in known]

    with ThreadPoolExecutor(10) as pool:
        results = list(pool.map(lambda t: (t[0], t[1], listing(t[1])), wanted))

    rows = [(by_size.get(a, 0), n, a, reads_locals_back(lines))
            for n, a, lines in results if unoptimised(lines)]
    rows.sort(reverse=True)

    total = sum(r[0] for r in rows)
    for size, name, address, strong in rows:
        print("%6d  0x%08x  %-44s %s" % (size, address, name, "strong" if strong else ""))
    print("%d functions, %d bytes" % (len(rows), total))


if __name__ == "__main__":
    main()
