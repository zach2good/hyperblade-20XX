"""Find a local pointer that is read but never written, which compiles and is wrong.

An offset conversion often starts by adding a typed local beside a char* alias:

    SoundEntry* pSnd;              declared
    pSnd = (SoundEntry*)pRecord;   assigned   <- forget this line and nothing complains

VC6 does not diagnose it, and the byte comparison can stay unchanged when the uninitialised
pointer happens to get the same register as the real one.

A write is an assignment or an address taken, and nothing else; passing the pointer to a
function is a read. The test is crude, and anything it cannot parse it leaves alone.

    python tools/check_unassigned_locals.py
"""

import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# `SoundEntry* pSnd;`. A `char* const p = ...` is not matched because it is assigned in place.
DECL = re.compile(r"^[ \t]+([A-Za-z_][A-Za-z0-9_]*)\s*\*+\s*([a-z][A-Za-z0-9_]*)\s*;\s*$", re.M)

# A body runs from a brace at column nought to the matching one.
BODY = re.compile(r"^\{\n(.*?)^\}", re.M | re.S)


def unassigned(text):
    """Every declared pointer in every body here that nothing ever writes."""
    found = []

    for body in BODY.finditer(text):
        block = body.group(1)

        for hit in DECL.finditer(block):
            name = hit.group(2)
            word = r"\b" + re.escape(name) + r"\b"

            # `&name` is a write, but `&name->field` takes a member's address and is not.
            if (re.search(word + r"\s*=[^=]", block)
                    or re.search(r"&\s*" + word + r"\s*(?![-.>])", block)):
                continue

            reads = len(re.findall(word, block))

            if reads > 1:
                line = text[:body.start(1) + hit.start()].count(chr(10)) + 1
                found.append((line, hit.group(1), name, reads - 1))

    return found


def main():
    total = 0

    for path in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True)):
        text = open(path, encoding="utf-8").read()

        for line, spelt, name, reads in unassigned(text):
            total = total + 1
            print("%s:%d  %s %s is read %d time(s) and never written"
                  % (os.path.basename(path), line, spelt, name, reads))

    print()
    print("%d local pointer(s) read without ever being assigned" % total)

    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
