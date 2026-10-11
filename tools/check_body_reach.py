"""Fail when a marked body reads at or past the end of its own class.

`new X` in the original is a `push <size>` and a call to operator new, so tools/object_sizes.py
reads class lengths out of the image. A body that reaches at or past its class's length means
either the class is too short or the body belongs to a derived class.

Only offsets applied to `this` or to the locals that hold it count (tools/field_names.py finds
those holders).

    python tools/check_body_reach.py

Exit code 1 unless every reach is either inside the class or listed in symbols/body_reach.csv
with the reason it cannot be settled yet.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")
REGISTER = os.path.join(ROOT, "symbols", "body_reach.csv")

# tools/field_names.py finds each body's holders of this, instead of a fixed list of names.
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import field_names as fn

BODY = re.compile(r"^[\w:*&<> ]*?([0-9A-Za-z_]+)::([~0-9A-Za-z_]+)\s*\([^;{]*\)\s*"
                  + chr(10) + r"?\s*\{", re.M)


def sourceNames(root):
    # Every file under root, recursively, as a path relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def sizes():
    """{class: the length the client allocates}, from tools/object_sizes.py."""
    out = {}
    text = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "object_sizes.py")],
                          capture_output=True, text=True).stdout

    for line in text.split(chr(10)):
        hit = re.match(r"\s+([0-9A-Za-z_]+)\s+client (0x[0-9a-f]+)", line)

        if hit:
            out[hit.group(1)] = int(hit.group(2), 16)

    return out


def accepted():
    found = {}

    if not os.path.exists(REGISTER):
        return found

    for line in open(REGISTER, encoding="utf-8"):
        line = line.strip()

        if not line or line.startswith("#") or line.startswith("class,"):
            continue

        parts = line.split(",", 2)

        if len(parts) >= 2:
            found[(parts[0], parts[1])] = parts[2] if len(parts) > 2 else ""

    return found


def main():
    known = sizes()
    allowed = accepted()
    rows = []

    for name in sorted(sourceNames(SRC)):
        if not name.endswith(".cpp"):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8",
                       errors="replace").read().replace(chr(13) + chr(10), chr(10))

        for hit in BODY.finditer(text):
            klass = hit.group(1)

            if klass not in known:
                continue

            end = text.find(chr(10) + "}" + chr(10), hit.end())
            body = text[hit.end():end if end > 0 else len(text)]
            deepest = 0

            for who in fn.holders(body):
                for word in re.findall(r"(?:" + re.escape(who) + r")\s*\+\s*"
                                       + r"(0x[0-9a-fA-F]+)", body):
                    deepest = max(deepest, int(word, 16))

            if deepest and deepest >= known[klass]:
                rows.append((klass, hit.group(2), known[klass], deepest, name))

    loud = [row for row in rows if (row[0], row[1]) not in allowed]

    for klass, method, size, deepest, name in sorted(loud):
        print("%s  %s::%s reaches 0x%x where the class is 0x%x"
              % (name, klass, method, deepest, size))

    print("%d class(es) have a length; %d body(ies) reach past one, %d of them registered"
          % (len(known), len(rows), len(rows) - len(loud)))

    if loud:
        print("Either the class is short or the body belongs further down. Settle it, or give")
        print("it a row in symbols/body_reach.csv saying what blocks that.")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
