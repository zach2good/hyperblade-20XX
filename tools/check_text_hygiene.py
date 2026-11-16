"""Fail on a control character in the tree's text files.

A script that writes source can put a raw control byte where an escape was meant, such as a
backspace for a regex's word boundary, a 0x01 for a group reference or a NUL for `\0`.
The byte is invisible in most editors and a reviewer reads past it. Tab, line feed and
carriage return are allowed. A file the attributes mark `-text` is skipped.

    python tools/check_text_hygiene.py
"""

import os
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

ALLOWED = {9, 10, 13}
CONTROL = set(range(32)) - ALLOWED | {127}


def tracked():
    """Every tracked file the attributes call text; a git failure stops the check rather than
    passing it."""
    out = subprocess.run(["git", "ls-files", "--eol"], capture_output=True, text=True, cwd=ROOT,
                         check=True)
    files = []
    for line in out.stdout.splitlines():
        flags, path = line.split("\t", 1)
        if "attr/-text" not in flags.split():
            files.append(path)

    return files


def main():
    faults = 0
    for path in tracked():
        with open(os.path.join(ROOT, path), "rb") as handle:
            data = handle.read()

        for number, line in enumerate(data.splitlines(True), 1):
            found = sorted({byte for byte in line if byte in CONTROL})
            if found:
                faults += 1
                print("%s:%d: control byte %s" % (path, number, ", ".join("0x%02x" % b for b in found)))

    print("%d line(s) with a control byte" % faults)
    return 1 if faults else 0


if __name__ == "__main__":
    raise SystemExit(main())
