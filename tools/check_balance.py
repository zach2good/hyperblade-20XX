"""Fail on a source file whose brackets do not balance.

A quick guard after a bulk rewrite: a dropped or repeated piece of a file shows as an unbalanced
bracket, in a second and without a build.

Each build configuration is read on its own, once with HYPERX_BRINGUP and once without, since
a brace may open in one `#ifdef` arm and close in another. A fault names the build it is in.

Comments, string literals and character literals are blanked first, so an apostrophe in prose
and a `"("` in a format string are not counted.

    python tools/check_balance.py

Exit code 1 when one is found.
"""
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")
LINE = chr(10)
SLASH = chr(92)

PAIRS = {")": "(", "]": "[", "}": "{"}


def sourceNames(root):
    # Sources live in subdirectories, so this walks. Names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def blanked(text):
    """The text with comments and literals replaced by spaces, so only code is counted."""
    out = list(text)
    at = 0

    while at < len(text):
        here = text[at]

        if here == "/" and text[at:at + 2] == "//":
            end = text.find(LINE, at)
            end = len(text) if end < 0 else end
        elif here == "/" and text[at:at + 2] == "/*":
            end = text.find("*/", at)
            end = len(text) if end < 0 else end + 2
        elif here in "'" + '"':
            end = at + 1

            while end < len(text) and text[end] != here:
                end += 2 if text[end] == SLASH else 1

            end = min(end + 1, len(text))
        else:
            at += 1
            continue

        for index in range(at, end):
            out[index] = " "

        at = end

    return "".join(out)


DIRECTIVE = __import__("re").compile(r"^\s*#\s*(\w+)\b(.*)$")
DEFINED = __import__("re").compile(r"^\s*(!?)\s*defined\s*(?:\(\s*(\w+)\s*\)|(\w+))\s*$")

# The two configurations the tree is built with.
CONFIGURATIONS = (("the measured build", frozenset()),
                  ("the bring-up build", frozenset(["HYPERX_BRINGUP"])))


def arm(rest, defines):
    """Whether a conditional's condition holds, taking the first arm when it cannot tell.

    Only `defined X` and its negation are read. An unknown condition takes its first arm, so
    at least one whole arm is counted.
    """
    hit = DEFINED.match(rest)

    if hit is None:
        return True

    name = hit.group(2) or hit.group(3)

    return (name in defines) != bool(hit.group(1))


def configured(text, defines):
    """The text with the arms this macro set does not compile blanked, line count kept.

    An inactive line becomes empty so line numbers hold. Directive lines are emptied too: an
    `#if`'s parentheses and a macro body's braces are not counted.
    """
    out = []
    stack = []
    carry = False

    for line in text.split(LINE):
        if carry:
            out.append("")
            carry = line.rstrip().endswith(SLASH)

            continue

        hit = DIRECTIVE.match(line)

        if hit is not None:
            name = hit.group(1)

            if name in ("if", "ifdef", "ifndef"):
                if name == "ifdef":
                    live = hit.group(2).strip() in defines
                elif name == "ifndef":
                    live = hit.group(2).strip() not in defines
                else:
                    live = arm(hit.group(2), defines)

                stack.append([live, live])
            elif name == "elif" and stack:
                live = not stack[-1][0] and arm(hit.group(2), defines)
                stack[-1] = [stack[-1][0] or live, live]
            elif name == "else" and stack:
                stack[-1] = [True, not stack[-1][0]]
            elif name == "endif" and stack:
                stack.pop()

            out.append("")
            carry = line.rstrip().endswith(SLASH)

            continue

        out.append(line if all(frame[1] for frame in stack) else "")

    return LINE.join(out)


def unbalanced(text):
    """The line of the first bracket that closes nothing or is never closed, or None."""
    text = blanked(text)
    stack = []

    for index, here in enumerate(text):
        if here in "([{":
            stack.append((here, index))
            continue

        if here not in PAIRS:
            continue

        if not stack or stack[-1][0] != PAIRS[here]:
            return text.count(LINE, 0, index) + 1, "closes nothing: " + here

        stack.pop()

    if stack:
        return text.count(LINE, 0, stack[0][1]) + 1, "never closed: " + stack[0][0]

    return None


def main():
    found = []

    for name in sorted(sourceNames(SRC)):
        if not name.endswith((".cpp", ".h")):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8",
                       errors="replace").read().replace(chr(13) + LINE, LINE)

        for label, defines in CONFIGURATIONS:
            fault = unbalanced(configured(text, defines))

            if fault is not None:
                found.append((name, fault[0], fault[1] + ", in " + label))

                break

    for row in found:
        print("%s:%d  %s" % row)

    print("%d file(s) do not balance" % len(found))

    if found:
        print("A bulk rewrite dropped or repeated a piece. Do not build on it; revert and fix.")

        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
