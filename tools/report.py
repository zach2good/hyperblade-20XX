"""The one place that opens build/report.json, and the one place that says when it is stale.

Tools that read build/report.json load it here, so every one of them refuses a report older than
the image it describes.

    from report import load           # the rows, with the check
    rows = load()                     # exits 1 and says how to regenerate if it is stale
    rows = load(onstale="warn")       # print the warning and carry on

The check compares the image's modification time with the report's. Regenerate the report
with

    python -m reccmp.tools.asmcmp --target HYPERX --json build/report.json --silent
"""
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPORT = os.path.join(ROOT, "build", "report.json")
IMAGE = os.path.join(ROOT, "build", "out", "HYPERX_sym.exe")
REGENERATE = ("  python -m reccmp.tools.asmcmp --target HYPERX --json build/report.json --silent")


def stale():
    """True when the image is newer than the report, so every figure in it is a build behind."""
    if not os.path.exists(REPORT) or not os.path.exists(IMAGE):
        return False

    return os.path.getmtime(IMAGE) > os.path.getmtime(REPORT)


def load(onstale="fail"):
    """The report's rows. `onstale` is 'fail' to exit, 'warn' to carry on, 'ignore' to say nothing."""
    if not os.path.exists(REPORT):
        sys.stderr.write("no build/report.json. Regenerate it:" + chr(10) + REGENERATE + chr(10))
        sys.exit(1)

    if stale() and onstale != "ignore":
        sys.stderr.write("STALE REPORT: the image is newer than build/report.json, so every"
                         + chr(10) + "figure here is the PREVIOUS build. Regenerate it:"
                         + chr(10) + REGENERATE + chr(10))

        if onstale == "fail":
            sys.exit(1)

    return json.load(open(REPORT, encoding="utf-8"))["data"]


def score(row):
    """A row's score as the headline counts it, which is not the `matching` field.

    A row with `effective: true` differs from the client only in ways reccmp judges harmless.
    It prints as `100.00%*` and counts as 100, while its `matching` is the raw instruction
    ratio. A tool that ranks on `matching` sends work at rows that are already done.
    """
    if row.get("effective"):
        return 100.0

    return float(row.get("matching", 0.0)) * 100.0
