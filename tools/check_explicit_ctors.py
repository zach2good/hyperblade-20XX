# Find single-argument constructors that are not `explicit`.
#
# Such a constructor is a silent conversion: `f(pSession)` compiles into `f(CPlayer(pSession))`
# wherever f takes a CPlayer. `explicit` changes overload resolution and emits nothing.
#
# A constructor that must stay implicit, such as `String(LPCSTR)`, carries a note in the comment
# block above it saying so, and the note settles it. The reason belongs beside the code.
#
# Not reported: copy constructors, and `X(void)`. A constructor with defaulted trailing arguments
# converts like a one-argument one, but the pattern reads it as taking several and misses it.
# The tree has none.
#
# Gates at zero.

import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

OPEN = re.compile(r"^[ \t]*(?:class|struct)\s+(?:__declspec\(\w+\)\s+)?(\w+)\b[^;\r\n]*$", re.M)


def noted(raw, at):
    """True when the comment block directly above the offset answers for the constructor."""
    kept = []

    for line in reversed(raw[:at].rstrip().split("\n")):
        if not line.strip().startswith("//"):
            break

        kept.append(line.strip()[2:].strip())

    note = " ".join(reversed(kept)).lower()

    return "explicit" in note or "implicit" in note or "conversion" in note


def main():
    os.chdir(ROOT)

    hits = []

    for path in sorted(glob.glob(os.path.join("src", "**", "*.h"), recursive=True)):
        shown = os.path.relpath(path, "src").replace(os.sep, "/")
        raw = open(path, encoding="utf-8", errors="replace").read()
        text = code(raw)

        for hit in OPEN.finditer(text):
            cls = hit.group(1)
            brace = text.find("{", hit.end())

            if brace < 0:
                continue

            depth = 0

            for end in range(brace, len(text)):
                if text[end] == "{":
                    depth += 1
                elif text[end] == "}":
                    depth -= 1

                    if depth == 0:
                        break

            for ctor in re.finditer(r"^[ \t]*(explicit\s+)?%s\s*\(\s*([^),;]+?)\s*\)"
                                    % re.escape(cls), text[brace:end], re.M):
                arg = ctor.group(2).strip()

                if ctor.group(1) or arg in ("void", ""):
                    continue

                if arg.startswith("const %s&" % cls) or arg.startswith("%s&" % cls):
                    continue

                if noted(raw, brace + ctor.start()):
                    continue

                hits.append((shown, text.count("\n", 0, brace + ctor.start()) + 1, cls, arg))

    for shown, at, cls, arg in hits:
        print("%-40s:%-5d %s(%s) converts silently" % (shown, at, cls, arg[:38]))

    print("%d single-argument constructor(s) that are not explicit and carry no note" % len(hits))

    return 1 if hits else 0


if __name__ == "__main__":
    sys.exit(main())
