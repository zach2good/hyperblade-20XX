# Finds a parameter the body never names and no `(void)` cast admits.
#
# Usage: python tools/unused_params.py [--list] [--apply FILE.cpp]
#   --list   print each bare parameter.
#   --apply  write the casts into one file, after the declarations at the head of each body.
# Exits 1 when any bare parameter remains.
#
# Signatures come from the image, so an unread argument is usually the client's ABI. A
# `(void)x;` or an unnamed parameter says so and emits no instruction; a named parameter the
# body never reads could also be a missed read.
# VC6 at this warning level does not emit C4100.
#
# A parameter named anywhere in the body, including an `#ifdef HYPERX_BRINGUP` block, is not
# reported. Only bodies whose brace sits on the line after the signature are read. That skips
# constructors with an initialiser list and signatures with function-pointer parameters.

import collections
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

BODY = re.compile(
    r"^(?P<head>[A-Za-z_][\w:<>,\*&\s]*?\b(?P<name>[A-Za-z_]\w*(?:::~?\w+)?)\s*\((?P<args>[^)]*)\))"
    r"\s*\r?\n\{\r?\n(?P<body>.*?)\r?\n\}", re.M | re.S)

# The name at the end of one parameter, with any array brackets after it.
PARAM = re.compile(r"\b([A-Za-z_]\w*)\s*(?:\[\s*\])?\s*$")

# A declaration line in the decompiler's shape: a type, a name, a semicolon, nothing else.
DECLARE = re.compile(r"^\s+[A-Za-z_][\w:<>\*&\s]*?\**\s*\w+(?:\[[^\]]*\])?;\s*$")

VOID_CAST = re.compile(r"^\s+\(void\)\w+;\s*$")

KEYWORDS = {"void", "int", "char", "float", "short", "unsigned", "long", "double", "const",
            "struct", "class", "signed", "bool"}

SKIP = ("jpeg/",)


def sources():
    for path in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True)):
        shown = os.path.relpath(path, os.path.join(ROOT, "src")).replace(os.sep, "/")

        if not shown.startswith(SKIP):
            yield path, "src/" + shown


def bare(hit):
    """The parameters this body never names, in declaration order."""
    args = hit.group("args").strip()

    if not args or args == "void":
        return []

    body = hit.group("body")
    out = []

    for one in args.split(","):
        # One identifier, or a tag and its name, is a type with no parameter name to cast.
        words = [w for w in re.findall(r"[A-Za-z_]\w*", one) if w not in ("const", "volatile")]

        if len(words) == 1 or (len(words) == 2 and words[0] in ("struct", "class", "union", "enum")):
            continue

        found = PARAM.search(one.strip())

        if not found or found.group(1) in KEYWORDS:
            continue

        name = found.group(1)

        if re.search(r"\b" + re.escape(name) + r"\b", body):
            continue

        out.append(name)

    return out


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv
    apply = None

    if "--apply" in sys.argv:
        apply = sys.argv[sys.argv.index("--apply") + 1]

    open_ones = collections.defaultdict(list)

    for path, shown in sources():
        text = open(path, encoding="utf-8", errors="replace").read()

        for hit in BODY.finditer(text):
            for name in bare(hit):
                open_ones[shown].append((hit.group("name"), name))

    total = sum(len(v) for v in open_ones.values())

    if show:
        for shown in sorted(open_ones):
            for who, name in open_ones[shown]:
                print("%-44s %s(%s)" % (shown, who, name))

    if apply:
        print("%d cast(s) written into %s" % (write_one(apply), apply))
        return 0

    print("%d parameter(s) the body never names and no (void) cast admits, over %d file(s)"
          % (total, len(open_ones)))

    return 1 if total else 0


def write_one(target):
    text = open(target, encoding="utf-8", newline="").read()
    # Keep the file's own line ending; src/ is mostly CRLF.
    nl = "\r\n" if "\r\n" in text else "\n"
    wrote = 0

    # Back to front, so an earlier body's offsets still hold after a later one is rewritten.
    for hit in reversed(list(BODY.finditer(text))):
        names = bare(hit)

        if not names:
            continue

        lines = hit.group("body").split(nl)
        at = 0

        while at < len(lines) and (DECLARE.match(lines[at])
                                   or VOID_CAST.match(lines[at])
                                   or not lines[at].strip()):
            at += 1

        # The casts join the run the declarations end with, one blank line clear of the code.
        while at > 0 and not lines[at - 1].strip():
            at -= 1

        added = ["    (void)%s;" % name for name in names]

        if at > 0 and not VOID_CAST.match(lines[at - 1]):
            added = [""] + added

        lines[at:at] = added
        body = nl.join(lines)
        text = text[:hit.start("body")] + body + text[hit.end("body"):]
        wrote += len(names)

    open(target, "w", encoding="utf-8", newline="").write(text)

    return wrote


if __name__ == "__main__":
    sys.exit(main())
