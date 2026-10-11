# Check inline assembly and naked routines against the register of original assembly.
#
# The tree writes assembly only where the original was hand-written assembly. Each such routine
# is a row in symbols/original_asm.csv, with the idioms that prove it, and carries three comment
# lines above its marker:
#
#   // Hand-written assembly in the original: <the idioms that prove it>
#   // What it does: <the routine in plain words>
#   // Why assembly: <what C cannot express>
#
# Each heading may run on over further comment lines, and each must carry at least MIN_WORDS
# words. `#pragma` lines between the block and the marker are skipped.
#
# Faults: an `__asm` or `__declspec(naked)` outside a marked function, in a function whose
# address is not in the register, or under a missing or thin block; and a register row with
# no evidence, or with no assembly in the tree at its address.
#
# Read only.

import csv
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

REGISTER = os.path.join("symbols", "original_asm.csv")

ASM = re.compile(r"\b_?_asm\b|__declspec\s*\(\s*naked\s*\)")
MARKER = re.compile(r"^\s*//\s*FUNCTION:\s*HYPERX\s+(0x[0-9a-fA-F]+)")
HEADINGS = ("Hand-written assembly in the original:", "What it does:", "Why assembly:")
MIN_WORDS = 4


def code_of(line):
    return line.split("//", 1)[0]


def read_register():
    rows = {}

    with open(REGISTER, encoding="utf-8", newline="") as handle:
        for row in csv.DictReader(line for line in handle if not line.startswith("#")):
            rows[row["address"].lower()] = row

    return rows


def body_end(lines, start):
    """The index of the line that closes the function whose declaration starts at start."""
    depth = 0
    opened = False

    for n in range(start, len(lines)):
        for ch in code_of(lines[n]):
            if ch == "{":
                depth += 1
                opened = True
            elif ch == "}":
                depth -= 1

                if opened and depth == 0:
                    return n

    return len(lines) - 1


def annotation_faults(lines, marker):
    """What is missing from the comment block above the marker at index marker."""
    block = []
    n = marker - 1

    while n >= 0:
        text = lines[n].strip()

        if text.startswith("#pragma"):
            n -= 1
            continue

        if not text.startswith("//") or MARKER.match(text):
            break

        block.insert(0, text[2:].strip())
        n -= 1

    found = {}
    current = None

    for text in block:
        heading = next((h for h in HEADINGS if text.startswith(h)), None)

        if heading is not None:
            current = heading
            found[current] = text[len(heading):].split()
        elif current is not None and text:
            found[current] += text.split()
        else:
            current = None

    faults = []

    for heading in HEADINGS:
        if heading not in found:
            faults.append("no `// %s` line above the marker" % heading)
        elif len(found[heading]) < MIN_WORDS:
            faults.append("`// %s` says too little" % heading)

    return faults


def main():
    os.chdir(ROOT)
    register = read_register()
    faults = []
    used = set()
    hits = 0

    for row in register.values():
        if len(row.get("evidence", "").split()) < MIN_WORDS:
            faults.append("%s: register row %s %s has no evidence"
                          % (REGISTER, row["address"], row["name"]))

    paths = []

    for suffix in ("*.cpp", "*.h", "*.c"):
        paths += sorted(glob.glob(os.path.join("src", "**", suffix), recursive=True))

    for path in paths:
        shown = path.replace(os.sep, "/")
        lines = open(path, encoding="utf-8", errors="replace").read().splitlines()
        functions = []

        for n, line in enumerate(lines):
            hit = MARKER.match(line)

            if hit:
                functions.append((n, body_end(lines, n + 1), hit.group(1).lower()))

        for n, line in enumerate(lines):
            if not ASM.search(code_of(line)):
                continue

            hits += 1
            owner = next((f for f in functions if f[0] < n <= f[1]), None)

            if owner is None:
                faults.append("%s:%d assembly outside a marked function" % (shown, n + 1))
                continue

            marker, end, address = owner

            if address not in register:
                faults.append("%s:%d assembly in %s, which is not in %s"
                              % (shown, n + 1, address, REGISTER))
                continue

            if address in used:
                continue

            used.add(address)

            for fault in annotation_faults(lines, marker):
                faults.append("%s:%d %s: %s" % (shown, marker + 1, address, fault))

    for address, row in sorted(register.items()):
        if address not in used:
            faults.append("%s: %s %s has no assembly in src" % (REGISTER, address, row["name"]))

    for fault in faults:
        print(fault)

    print("%d assembly line(s) in %d registered routine(s), %d fault(s)"
          % (hits, len(used), len(faults)))

    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
