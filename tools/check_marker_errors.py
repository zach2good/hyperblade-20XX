"""Reads the marker errors reccmp prints and fails on the ones that are claims about the binary.

`--silent` suppresses reccmp's `[ERROR]` lines, and every documented invocation passes it, so a
false annotation is dropped before scoring with no visible symptom. Three kinds of error exist.
Two are assertions about the original image that the image refutes, and both fail here:

    Data at 0x... does not match string '...'          the string is not there
    Invalid address 0x... on ... annotation in file    the address is not in the image

The third is benign: the same string literal is annotated in more than one file, and reccmp
keeps one on purpose. It is counted and printed, and does not fail.

An error that matches none of the three fails, so a new kind from a newer reccmp cannot arrive
silently.

    python tools/check_marker_errors.py

Takes about six seconds: it runs the score itself, because the errors appear while annotations
are ingested and are not written to the report. Its report goes to a temporary file, so
`build/report.json` is left alone.
"""
import os
import re
import subprocess
import sys
import tempfile
from collections import Counter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT = os.path.join(ROOT, "reccmp-project.yml")

BENIGN = re.compile(
    r"^Dropped duplicate address 0x[0-9a-f]+ on \w+ annotation in file: ")

FATAL = [
    ("string does not match the binary",
     re.compile(r"^Data at 0x[0-9a-f]+ does not match string ")),
    ("address is not in the image",
     re.compile(r"^Invalid address 0x[0-9a-f]+ on \w+ annotation in file: ")),
]

BENIGN_KIND = "duplicate address dropped"
UNKNOWN_KIND = "unrecognised error kind"


def classify(message):
    for kind, pattern in FATAL:
        if pattern.match(message):
            return kind, True

    if BENIGN.match(message):
        return BENIGN_KIND, False

    return UNKNOWN_KIND, True


if not os.path.exists(PROJECT):
    print("%s missing; this must run inside a configured recomp tree" % PROJECT)
    sys.exit(2)

handle, report = tempfile.mkstemp(suffix=".json", prefix="marker_errors_")
os.close(handle)

try:
    run = subprocess.run(
        [sys.executable, "-m", "reccmp.tools.asmcmp",
         "--target", "HYPERX", "--json", report, "--no-color"],
        cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        encoding="utf-8", errors="replace")
finally:
    if os.path.exists(report):
        os.remove(report)

if run.returncode != 0:
    print(run.stdout)
    print("asmcmp exited %d; the score must run before its errors can be read"
          % run.returncode)
    sys.exit(2)

failures = []
counts = Counter()

for line in run.stdout.splitlines():
    if not line.startswith("[ERROR] "):
        continue

    message = line[len("[ERROR] "):].strip()
    kind, fatal = classify(message)
    counts[kind] += 1

    if fatal:
        failures.append((kind, message))

for kind, message in failures:
    print("%-34s %s" % (kind, message))

for kind in sorted(counts):
    print("%6d  %s" % (counts[kind], kind))

print("%d marker errors, %d of them a false or impossible claim"
      % (sum(counts.values()), len(failures)))

sys.exit(1 if failures else 0)
