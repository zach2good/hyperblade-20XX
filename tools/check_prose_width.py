"""Fail on a prose line wider than 100 columns that could wrap.

Prose is a Markdown line, a `#` comment in a register under symbols/ or a Python tool, a Python
tool's module docstring, and a `//` comment in src/. Comment text indented past the marker's one
space, and docstring text indented at all, is an example or a list continuation, and is skipped. A
line counts only when it has a space to break at by column 100, past its marker and first word and
outside a code span, so a lone long identifier or path passes. Tables, code fences and lines holding
a URL are skipped.

    python tools/check_prose_width.py
"""

import os
import re
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

WIDTH = 100

URL = re.compile(r"https?://")

CODE_SPAN = re.compile(r"`[^`]*`")


def tracked():
    """Every tracked file; a git failure stops the check rather than passing it."""
    out = subprocess.run(["git", "ls-files"], capture_output=True, text=True, cwd=ROOT, check=True)
    return out.stdout.splitlines()


def prose(path, lines):
    """Answers (number, line) for each prose line in a file of this kind."""
    if path.endswith(".md"):
        fenced = False
        for number, line in enumerate(lines, 1):
            if line.lstrip().startswith("```"):
                fenced = not fenced
                continue

            # A line indented four spaces or a tab is an indented code block.
            if not fenced and not line.lstrip().startswith("|") and not line.startswith(("    ", "\t")):
                yield number, line

        return

    if path.startswith("src/") and path.endswith((".cpp", ".h")):
        marker = "//"
    elif path.startswith(("symbols/", "tools/")) and path.endswith((".csv", ".py")):
        marker = "#"
    else:
        return

    # A Python tool's module docstring is prose at column nought; its indented lines are examples.
    if path.endswith(".py") and lines and lines[0].startswith('"""'):
        for number, line in enumerate(lines, 1):
            if number > 1 and line.startswith('"""'):
                break

            if not line.startswith((" ", "\t")):
                yield number, line

            # A one-line docstring closes on its own first line.
            if line.rstrip().endswith('"""') and (number > 1 or len(line.strip()) > 3):
                break

    # Text indented past the marker's one space is an example or a list continuation.
    for number, line in enumerate(lines, 1):
        text = line.lstrip()
        if text.startswith(marker) and not text[len(marker):].startswith(("  ", " \t")):
            yield number, line


def breakable(line):
    """Answers whether the line is too wide and has a space to break at, past its leading marker
    and first word, and outside a code span."""
    if len(line) <= WIDTH:
        return False

    masked = CODE_SPAN.sub(lambda span: span.group(0).replace(" ", "_"), line)
    first = re.match(r"\s*\S+\s+\S+", masked)
    return first is not None and masked.rfind(" ", first.end(), WIDTH + 1) >= 0


def main():
    faults = 0
    for path in tracked():
        if path.startswith("third_party/"):
            continue

        with open(os.path.join(ROOT, path), encoding="utf-8", errors="replace") as handle:
            lines = handle.read().splitlines()

        for number, line in prose(path, lines):
            if breakable(line) and not URL.search(line):
                faults += 1
                print("%s:%d: %d columns" % (path, number, len(line)))

    print("%d prose line(s) over %d columns" % (faults, WIDTH))
    return 1 if faults else 0


if __name__ == "__main__":
    raise SystemExit(main())
