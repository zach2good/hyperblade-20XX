"""Blank the comments out of C++ source, keeping every offset, for tools that grep code.

String and character literals are read as literals, so a "//" inside one is not a comment.
Comments become spaces and keep their line breaks, so a line number or an offset into the
result is one into the file.

    from comments import code
    text = code(source)                  # comments blanked
    text = code(source, strings=True)    # literal contents blanked too, quotes kept
"""
import re

LEXEME = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.S)


def blank(text):
    return re.sub(r"[^\r\n]", " ", text)


def code(text, strings=False):
    def one(match):
        lexeme = match.group(0)

        if lexeme[0] not in "\"'":
            return blank(lexeme)

        if strings:
            return lexeme[0] + blank(lexeme[1:-1]) + lexeme[-1]

        return lexeme

    return LEXEME.sub(one, text)


if __name__ == "__main__":
    sample = 'a = "x // y"; // gone\nb = \'/\'; /* two\nlines */ c = "q";'
    assert code(sample) == 'a = "x // y";        \nb = \'/\';       \n         c = "q";'
    assert code(sample, strings=True) == 'a = "      ";        \nb = \' \';       \n         c = " ";'
    assert len(code(sample)) == len(sample)
    print("comments.py: self-check passed")
