# Checks two declaration rules: one name per declaration, and no alternative operator tokens.
#
#   One name per declaration. `Thing* pA, pB;` declares one pointer and one object.
#
#   No alternative operator tokens. `and`, `or`, `not` and their siblings are spelled `&&`,
#   `||` and `!`.
#
# Two exclusions:
#
#   src/jpeg/ is the IJG library carried verbatim, so it is not rewritten.
#
#   Inline assembly is not C++: `or bh, 1` and `xor ecx, ecx` are x86 mnemonics.
#
# Gates at zero.

import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Character literals too, in one pass, so '"' does not open a string.
STRING = re.compile(r'"(?:[^"\\]|\\.)*"|\'(?:[^\'\\\n]|\\.)+\'')
ASM_BLOCK = re.compile(r"__asm\s*\{[^{}]*\}", re.S)
ASM_LINE = re.compile(r"__asm[^\r\n]*")
ALT = re.compile(r"(?<![\w.])(not|and|or|bitand|bitor|xor|compl|not_eq)(?![\w.])")
# A declaration statement naming two or more variables. Indented, so a top-level array
# initialiser row cannot match, and no parenthesis or equals, so a call or an assignment
# cannot either.
MULTI = re.compile(r"^[ \t]+((?:const |static |volatile |unsigned |signed |struct |class )*"
                   r"[A-Za-z_][\w:]*)\s*\*?\s*(\w+\s*,\s*\*?\w+(?:\s*,\s*\*?\w+)*)\s*;\s*$", re.M)

KEYWORDS = ("return", "case", "delete", "goto", "else", "for", "while", "if", "operator",
            "sizeof")


def blank(match):
    """The match replaced by spaces, so every offset after it still lines up."""
    return re.sub(r"[^\r\n]", " ", match.group(0))


def main():
    os.chdir(ROOT)

    multi = []
    alt = []

    for path in (sorted(glob.glob(os.path.join("src", "**", "*.cpp"), recursive=True))
                 + sorted(glob.glob(os.path.join("src", "**", "*.h"), recursive=True))):
        shown = os.path.relpath(path, "src").replace(os.sep, "/")

        if shown.startswith("jpeg/"):
            continue

        text = code(open(path, encoding="utf-8", errors="replace").read())

        for hit in MULTI.finditer(text):
            if hit.group(1).split()[-1] in KEYWORDS:
                continue

            multi.append((shown, text.count("\n", 0, hit.start()) + 1,
                          hit.group(0).strip()[:56]))

        bare = ASM_LINE.sub(blank, ASM_BLOCK.sub(blank, STRING.sub('""', text)))

        for hit in ALT.finditer(bare):
            alt.append((shown, bare.count("\n", 0, hit.start()) + 1, hit.group(1)))

    for shown, at, what in multi:
        print("MULTI %-36s:%-6d %s" % (shown, at, what))

    for shown, at, what in alt:
        print("ALT   %-36s:%-6d `%s` where the tree writes the symbol" % (shown, at, what))

    print("%d declaration(s) naming more than one variable, %d alternative operator token(s), "
          "outside src/jpeg/ and inline assembly" % (len(multi), len(alt)))

    return 1 if multi or alt else 0


if __name__ == "__main__":
    sys.exit(main())
