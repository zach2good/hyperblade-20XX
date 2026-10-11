# Find plan handler id notes that sit on another handler's body.
#
# A note of the shape "id_ACF_Name in DATA/ENUMIDS.H ... registers this body at N" names the
# table index its body is stored at, and src/planvm/planvm.cpp stores every handler by
# index. This checks that the body under each note is the one registered at N. A fold or sort
# that moves bodies can rotate the notes onto their neighbours.
#
# The action table takes the id as its index and the opcode table takes the id two slots on;
# the note states the index either way, so only the index is compared.
#
# Read only.

import glob
import os
import re
import sys

import plan_tables

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TABLE = {"ACF": "g_pfnPlanActionHandlers", "DCF": "g_pfnPlanOpcodeHandlers"}
NOTE = re.compile(r"^// id_(ACF|DCF)_\w+ in DATA/ENUMIDS\.H")
AT = re.compile(r"registers this body at (\d+)")
BODY = re.compile(r"^[\w\s\*]*?\w+::(\w+)\(")


def main():
    registered = dict(((table, n), body.split("::")[-1]) for table, n, body in plan_tables.registrations())

    checked = 0
    bad = 0
    for path in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True)):
        with open(path, encoding="utf-8", errors="replace") as f:
            lines = f.read().split("\n")
        i = 0
        while i < len(lines):
            m = NOTE.match(lines[i])
            if not m:
                i += 1
                continue
            end = i
            while end < len(lines) and lines[end].startswith("//"):
                end += 1
            at = AT.search(" ".join(lines[i:end]))
            j = end
            while j < len(lines) and not BODY.match(lines[j]):
                j += 1
            here = BODY.match(lines[j]).group(1) if j < len(lines) else None
            want = registered.get((TABLE[m.group(1)], int(at.group(1)))) if at else None
            if at:
                checked += 1
                if want != here:
                    bad += 1
                    print("%s:%d  note for index %s is on %s; %s is registered there"
                          % (os.path.relpath(path, ROOT), i + 1, at.group(1), here, want))
            i = end

    print("%d id note(s) name a table index, %d on the wrong body" % (checked, bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
