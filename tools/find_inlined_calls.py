"""List calls the original makes that this build inlines instead.

For every function below exact, compare the calls and tail jumps on both sides of the diff. A
callee the original calls and this build does not, defined in the same source file or inline in a
header, is one VC6 inlined here. The game builds at /O2, which inlines only a callee marked
`inline` or defined in its class, so drop the keyword or move the body out of the class, then
rebuild and rescore. A header inline that the original inlines in some callers and calls in others
cannot be split this way.

    python tools/find_inlined_calls.py

Prints one line per function: score, address, name and the callees to keep out of line. Reads
build/report.json and runs reccmp's asmcmp once per function, so it takes several minutes.
"""

import glob
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

MARKER = re.compile(r"// FUNCTION: HYPERX (0x[0-9a-fA-F]+)")
MISSING = re.compile(r":\s*-(?:call|jmp) ([\w:~`'@$?]+) \(FUNCTION\)")
PRESENT = re.compile(r":\s*\+(?:call|jmp) ([\w:~`'@$?]+) \(FUNCTION\)")


def homes():
    """Each marked address and the file its body is in."""
    found = {}

    for suffix in ("*.cpp", "*.h"):
        for path in glob.glob(os.path.join("src", "**", suffix), recursive=True):
            text = open(path, encoding="utf-8", errors="replace").read()

            for hit in MARKER.finditer(text):
                found[int(hit.group(1), 16)] = path.replace(os.sep, "/")

    return found


def main():
    os.chdir(ROOT)
    home = homes()
    rows = json.load(open(os.path.join("build", "report.json")))["data"]
    byname = {row["name"]: int(str(row["address"]), 16) for row in rows}
    found = 0

    for row in rows:
        address = int(str(row["address"]), 16)

        if (row.get("matching") or 0) >= 1 or address not in home:
            continue

        out = subprocess.run([sys.executable, "-m", "reccmp.tools.asmcmp", "--target", "HYPERX",
                              "--verbose", hex(address)],
                             capture_output=True, text=True, errors="ignore").stdout
        out = re.sub(r"\x1b\[[0-9;]*m", "", out)
        lost = set(MISSING.findall(out)) - set(PRESENT.findall(out))
        inlined = []

        for callee in sorted(lost):
            where = home.get(byname.get(callee, -1), "")

            if where == home[address] or where.endswith(".h"):
                inlined.append(callee)

        if inlined:
            found += 1
            print("%6.2f  %s  %s  %s" % ((row.get("matching") or 0) * 100, hex(address),
                                         row["name"], " ".join(inlined)), flush=True)

    print("%d function(s) call something this build inlines" % found)

    return 0


if __name__ == "__main__":
    sys.exit(main())
