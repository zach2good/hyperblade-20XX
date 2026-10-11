# Fails on a local that is initialised once, never written again, and not declared const.
#
#     python tools/const_locals.py                         # the ten files with the most
#     python tools/const_locals.py --list                  # every candidate
#     python tools/const_locals.py --apply src/x/file.cpp  # add const in one file
#
# Exits 1 when the report finds any.
#
# Only a declaration that initialises counts; moving a later assignment up to the declaration
# can move a register. A pointer takes `T* const p`, a scalar takes `const T x`. A local of class
# type is left alone, because few member functions here are declared const. Also left out: a
# name whose address is taken (an out parameter), a name written after its declaration, and
# src/jpeg/.
#
# The scan runs on a copy with comments blanked to spaces, so its offsets are the file's offsets.

import collections
import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

OPEN = re.compile(r"^\{$", re.M)
# The type and the name must be on one line; `\s+` would cross a newline and read a label such
# as `default:` as a type.
INIT = re.compile(r"^[ \t]+((?:const[ \t]+)?(?:unsigned[ \t]+|signed[ \t]+)?[\w:]+[ \t]*\**)"
                  r"[ \t]+(\w+)[ \t]*=\s*([^;]+);", re.M)

# Only scalars and pointers take const. Few member functions here are const, so a const value
# of class type would refuse its first member call.
SCALARS = ("char", "short", "int", "long", "float", "double", "bool", "size_t", "wchar_t",
           "unsigned", "signed", "DWORD", "WORD", "BYTE", "UINT", "BOOL", "LONG", "ULONG",
           "SHORT", "USHORT", "FLOAT", "HRESULT", "LRESULT", "WPARAM", "LPARAM", "ATOM",
           "COLORREF", "HDC", "HWND", "HINSTANCE", "HANDLE", "HBITMAP", "HMENU", "HFONT")
# A write is the name anywhere with a write operator before or after it, including inside a
# condition such as `while (n-- && *cp)`. `=[^=]` keeps `n == 0` out.
WRITE = (r"(?<![\w.>])%s\s*(?:=[^=]|\+\+|--|\+=|-=|\*=|/=|&=|\|=|\^=|<<=|>>=)"
         r"|(?:\+\+|--)\s*%s(?![\w])")
TAKEN = r"&\s*%s\b"

KEYWORDS = ("return", "else", "case", "delete", "new", "throw")


def bodies(text):
    """(start, end) of every brace-balanced function body, by the opener this tree writes."""
    for hit in OPEN.finditer(text):
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
                    yield hit.start(), i
                    break


def candidates(text):
    """Each (offset of the type, name) a const would be free on."""
    found = []

    for start, end in bodies(text):
        body = text[start:end]

        for hit in INIT.finditer(body):
            kind = hit.group(1).strip()
            name = hit.group(2)

            if kind.startswith("const") or kind.split()[0] in KEYWORDS:
                continue

            if re.search(WRITE % (re.escape(name), re.escape(name)),
                         body[hit.end():], re.M):
                continue

            if re.search(TAKEN % re.escape(name), body):
                continue

            # The pointer is what is never written, so it takes `T* const p`; `const T* p` would
            # make the pointee const.
            if kind.endswith("*"):
                found.append((start + hit.end(1), name, kind, " const"))
            elif kind.split()[-1] in SCALARS:
                found.append((start + hit.start(1), name, kind, "const "))

    return found


def sources():
    for path in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True)):
        shown = os.path.relpath(path, ROOT).replace(os.sep, "/")

        if shown.startswith("src/jpeg/"):
            continue

        yield path, shown


def main():
    show = "--list" in sys.argv
    apply = None

    if "--apply" in sys.argv:
        apply = sys.argv[sys.argv.index("--apply") + 1].replace("\\", "/")

    total = 0
    byFile = collections.Counter()

    for path, shown in sources():
        raw = open(path, encoding="utf-8", newline="").read()
        found = candidates(code(raw.replace("\r\n", "\n")))

        if not found:
            continue

        total += len(found)
        byFile[shown] = len(found)

        if show:
            for at, name, kind, where in found:
                print("%-46s %-24s %s %s" % (shown, name, kind, where.strip()))

        if apply and shown == apply:
            flat = raw.replace("\r\n", "\n")

            for at, name, kind, where in reversed(found):
                flat = flat[:at] + where + flat[at:]

            if "\r\n" in raw:
                flat = flat.replace("\n", "\r\n")

            open(path, "w", encoding="utf-8", newline="").write(flat)
            print("%s: %d local(s) took const" % (shown, len(found)))

            return 0

    if apply:
        print("%s has no candidate, or is not a source this reads" % apply)

        return 1

    if not show:
        for shown, n in byFile.most_common(10):
            print("   %-46s %d" % (shown, n))

    print("%d local(s) initialised once, never written again, and not declared const, "
          "over %d file(s)" % (total, len(byFile)))

    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
