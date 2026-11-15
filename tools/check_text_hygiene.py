"""Fail on a control character in the tree's text files.

A script that writes source can put a raw control byte where an escape was meant, such as a
backspace for a regex's word boundary or a 0x01 for a group reference. The byte is invisible in
most editors and a reviewer reads past it. Tab, line feed and carriage return are allowed.

    python tools/check_text_hygiene.py
"""

import os
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

TEXT = (".c", ".cpp", ".h", ".md", ".csv", ".py", ".txt", ".json", ".yml", ".yaml", ".cmake")
ALLOWED = {9, 10, 13}
CONTROL = set(range(32)) - ALLOWED | {127}


def tracked():
    out = subprocess.run(["git", "ls-files"], capture_output=True, text=True, cwd=ROOT)
    return [path for path in out.stdout.splitlines() if path.endswith(TEXT)]


def main():
    faults = 0
    for path in tracked():
        with open(os.path.join(ROOT, path), "rb") as handle:
            for number, line in enumerate(handle, 1):
                found = sorted({byte for byte in line if byte in CONTROL})
                if found:
                    faults += 1
                    print("%s:%d: control byte %s" % (path, number, ", ".join("0x%02x" % b for b in found)))

    print("%d line(s) with a control byte" % faults)
    return 1 if faults else 0


if __name__ == "__main__":
    raise SystemExit(main())
