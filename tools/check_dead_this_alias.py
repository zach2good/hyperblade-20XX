"""Report an alias of `this` that its own function never reads.

`char* const pObj = (char*)this;` above a body that never uses pObj is decompiler scaffolding.
This reports rather than deletes: a dead local can still change register allocation, so removal
must be measured. An alias measured and kept says `KEEP.` in the comment above it and is skipped.

  - The body is bounded by its own braces.
  - An alias any #define in the file mentions is skipped, since a macro may read it.

Exit code 1 when any dead alias is found.

    python tools/check_dead_this_alias.py
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")
LINE = chr(10)

# Any pointer type, const or not, cast or not.
ALIAS = re.compile(r"^([ \t]*)[0-9A-Za-z_]+\*\s+(?:const\s+)?([0-9A-Za-z_]+)\s*=\s*"
                   r"(?:\([0-9A-Za-z_]+\*\)\s*)?this;[ \t]*$", re.M)

# The alias split in two: `char* pThis;` and later `pThis = (char*)this;`. The declaration
# spends one more mention of the name, so its read threshold is two.
ALIAS_SPLIT = re.compile(r"^([ 	]*)([0-9A-Za-z_]+)\s*=\s*"
                         r"(?:\([0-9A-Za-z_]+\*\)\s*)?this;[ 	]*$", re.M)

KEEP = "KEEP."


def sourceNames(root):
    # Sources live in subdirectories, so this walks. Names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def bodies(text):
    """(start, end) of every brace-balanced FUNCTION body.

    The brace must follow a closing parenthesis, so an `extern "C"` block or a column-zero
    `struct` does not count as a body.
    """
    found = []

    for hit in re.finditer(r"^\{$", text, re.M):
        above = text.rfind(")", 0, hit.start())

        if above < 0 or "\n" in text[above:hit.start()].strip():
            continue

        depth = 0

        for i in range(hit.start(), len(text)):
            if text[i] == "{":
                depth += 1
            elif text[i] == "}":
                depth -= 1

                if depth == 0:
                    found.append((hit.start(), i))
                    break

    return found


def dead():
    """Every alias of this that nothing in its own body, and no macro, reads."""
    found = []

    for name in sorted(sourceNames(SRC)):
        if not name.endswith(".cpp"):
            continue

        path = os.path.join(SRC, name)
        text = open(path, encoding="utf-8", errors="replace").read()
        text = text.replace(chr(13) + LINE, LINE)
        spans = bodies(text)
        macros = LINE.join(re.findall(r"^#define .*$", text, re.M))

        for pattern, mentions in ((ALIAS, 1), (ALIAS_SPLIT, 2)):
          for hit in pattern.finditer(text):
            span = next((s for s in spans if s[0] <= hit.start() <= s[1]), None)

            if span is None:
                continue

            local = hit.group(2)
            word = r"(?<![0-9A-Za-z_])%s(?![0-9A-Za-z_])" % local

            # Only a name this body declares is an alias; a store of this into a global is not.
            if mentions == 2:
                decl = r"^[ 	]*[0-9A-Za-z_]+\*[ 	]+%s[ 	]*;[ 	]*$" % local
                if not re.search(decl, text[span[0]:span[1]], re.M):
                    continue

            if len(re.findall(word, text[span[0]:span[1]])) > mentions:
                continue

            if re.search(word, macros):
                continue

            if KEEP in text[max(0, hit.start() - 300):hit.start()]:
                continue

            found.append((name, local, text.count(LINE, 0, hit.start()) + 1))

    return found


def main():
    bad = dead()

    for name, local, line in bad:
        print("DEAD ALIAS src/%s:%d  %s is assigned this and never read" % (name, line, local))

    print("%d alias(es) of this that their own body never reads" % len(bad))

    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
