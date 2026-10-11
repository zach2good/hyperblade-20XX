# Report reccmp markers that were in HEAD and are no longer anywhere under src.
#
# tools/check_shrinkage.py catches a file overwritten with something smaller; this compares
# marker sets, so it also catches an overwrite that grew the file.
#
# STUB to FUNCTION and GLOBAL to VTABLE at the same address are promotions: reported, not
# failed. Any other change of kind, such as FUNCTION to STUB, fails.
#
#   python tools/check_lost_markers.py
#
# Exit code 1 when any marker is lost or changed.

import re
import subprocess
import sys

MARKER = re.compile(r"//\s*(FUNCTION|STUB|LIBRARY|GLOBAL|VTABLE):\s*HYPERX\s+(0x[0-9a-fA-F]+)")


def markers(text):
    return set((kind, addr.lower()) for kind, addr in MARKER.findall(text))


def run(args):
    return subprocess.run(args, capture_output=True, text=True).stdout


head = set()
for path in run(["git", "ls-tree", "-r", "--name-only", "HEAD", "src"]).split():
    head |= markers(run(["git", "show", "HEAD:" + path]))

now = set()
for path in run(["git", "ls-files", "src"]).split() + run(["git", "ls-files", "-o", "--exclude-standard", "src"]).split():
    try:
        now |= markers(open(path, encoding="utf-8").read())
    except OSError:
        pass

head_kinds = {}
for kind, addr in head:
    head_kinds.setdefault(addr, set()).add(kind)

now_kinds = {}
for kind, addr in now:
    now_kinds.setdefault(addr, set()).add(kind)

lost = []
promoted = []

for kind, addr in sorted(head - now):
    if addr not in now_kinds:
        lost.append(("LOST", kind, addr))
    elif kind == "STUB" and "FUNCTION" in now_kinds[addr]:
        promoted.append((addr, "FUNCTION"))
    elif kind == "GLOBAL" and "VTABLE" in now_kinds[addr]:
        promoted.append((addr, "VTABLE"))
    else:
        lost.append(("CHANGED", kind, addr))

for label, kind, addr in lost:
    print("%s %s %s" % (label, kind, addr))
for addr, kind in promoted:
    print("promoted to %s: %s" % (kind, addr))

print("%d markers in HEAD, %d lost, %d promoted" % (len(head), len(lost), len(promoted)))
sys.exit(1 if lost else 0)
