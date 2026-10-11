"""Fail on an explicit destructor call that is not qualified with its class.

`p->~Widget()` on a class whose destructor is virtual is a vtable call: VC6 pushes the flag,
loads the table and calls slot zero. `p->Widget::~Widget()` is a direct call to that class's own
destructor body. Every explicit destructor call in this tree means the second: the caller is a
scalar deleting destructor, a placement teardown or a static's exit thunk, and each runs one
class's body.

`delete p` is how this tree asks for the virtual one, and it is not reported.

    python tools/check_dtor_calls.py

Exit code 1 when one is found.
"""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

# An explicit destructor call with nothing but `->` or `.` in front of the tilde.
UNQUALIFIED = re.compile(r"(?<![0-9A-Za-z_:])(?:->|" + chr(92) + r".)" + chr(92)
                         + r"s*~([0-9A-Za-z_]+)" + chr(92) + r"s*" + chr(92) + r"(" + chr(92) + r")")


def sourceNames(root):
    # Walk subdirectories; names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def main():
    found = []

    for name in sorted(sourceNames(SRC)):
        if not name.endswith((".cpp", ".h")):
            continue

        path = os.path.join(SRC, name)
        text = open(path, encoding="utf-8", errors="replace").read()
        text = text.replace(chr(13) + chr(10), chr(10))

        for match in UNQUALIFIED.finditer(text):
            line = text.count(chr(10), 0, match.start()) + 1
            found.append((name, line, match.group(1)))

    for name, line, klass in found:
        print("%s:%d  ~%s() is called without its class" % (name, line, klass))

    print("%d explicit destructor call(s) missing the qualification" % len(found))

    if found:
        print("Write it `p->%s::~%s()`. Unqualified it is a call through slot zero, which is"
              % ("Klass", "Klass"))
        print("three instructions where the client has one.")

        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
