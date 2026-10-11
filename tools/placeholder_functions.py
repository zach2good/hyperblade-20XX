# Lists functions and methods whose name is a placeholder, with their callers and what each touches.
#
#   python tools/placeholder_functions.py            # the summary line
#   python tools/placeholder_functions.py --list     # one row per name, with callers and touches
#   python tools/placeholder_functions.py --max N    # also fail when there are more than N
#
# A name is a placeholder when it embeds an address or an index, as InitializeGlobal004CB370 or
# StaticDtor00000007 did, when it is a Ghidra default such as FUN_0044b0a0, or when it names only
# a table position or a resource number, as Slot06, Virtual12 and Dialog025C did. Only a name
# that a declaration or a definition gives counts; a call or a comment does not.
#
# The callers are the marked functions that use the name outside its own body. What a function
# touches is the globals and the functions its body names.
#
# Read only.

import collections
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from placeholder_globals import code, owners  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# A hex run of five or more characters, three of them digits, is an address or an index.
HEX_RUN = re.compile(r"[0-9A-Fa-f]{5,}")
POSITION = re.compile(r"(?:Slot|Virtual|VSlot|Method|Func|Sub|Unk|Unknown)_?\d+"
                      r"|Dialog[0-9A-Fa-f]{3,4}")
GHIDRA = re.compile(r"(?:thunk_)?(?:FUN|SUB|LAB|thunk_FUN)_[0-9A-Fa-f]+")

KEYWORDS = {"return", "if", "while", "for", "switch", "else", "case", "delete", "new", "sizeof",
            "throw", "do", "goto"}

# A declaration or a definition: a type, perhaps a calling convention, a qualified name, a '('.
DECL = re.compile(r"^\s*(?:(?:virtual|static|extern|inline|friend|const|unsigned|signed|struct|"
                  r"class)\s+)*([\w:]+)[\s*&]+(?:const\s+)?"
                  r"(?:(?:__\w+|CALLBACK|WINAPI|APIENTRY|PASCAL)\s+)?[\s*&]*"
                  r"((?:\w+::)*)(\w+)\s*\(")

CALL = re.compile(r"\b(\w+)\s*\(")
GLOBAL = re.compile(r"\b[gs]_\w+")


def placeholder(name):
    if GHIDRA.fullmatch(name) or POSITION.fullmatch(name):
        return True

    for hit in HEX_RUN.finditer(name):
        run = hit.group(0)

        # A run can start with hex letters that end the word before it, as Unrolled192 does.
        if hit.start() == 0 or name[hit.start() - 1] != "_":
            run = run.lstrip("ABCDEFabcdef")

        if len(run) >= 5 and sum(ch.isdigit() for ch in run) >= 3:
            return True

    return False


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv
    ceiling = None

    if "--max" in sys.argv:
        ceiling = int(sys.argv[sys.argv.index("--max") + 1])

    paths = sorted(glob.glob(os.path.join("src", "**", "*.h"), recursive=True)
                   + glob.glob(os.path.join("src", "**", "*.cpp"), recursive=True))

    texts = {}
    declared = collections.defaultdict(set)

    for path in paths:
        shown = os.path.relpath(path, "src").replace(os.sep, "/")
        raw = open(path, encoding="utf-8", errors="replace").read().replace("\r\n", "\n")
        text = code(raw)
        texts[shown] = (raw, text)

        for line in text.split("\n"):
            hit = DECL.match(line)

            if hit is None or hit.group(1) in KEYWORDS:
                continue

            if placeholder(hit.group(3)):
                declared[hit.group(3)].add(shown)

    callers = collections.defaultdict(collections.Counter)
    touches = collections.defaultdict(set)

    for shown, (raw, text) in texts.items():
        lines = text.split("\n")
        where = owners(raw)

        for number, line in enumerate(lines):
            here = where[number].split("::")[-1]
            own = here in declared

            for hit in CALL.finditer(line):
                name = hit.group(1)

                if own and name != here and name not in KEYWORDS:
                    touches[here].add(name + "()")

                if name not in declared or name == here:
                    continue

                decl = DECL.match(line)

                if decl is not None and decl.group(3) == name:
                    continue

                callers[name]["%s %s" % (shown, where[number])] += 1

            if own:
                for hit in GLOBAL.finditer(line):
                    touches[here].add(hit.group(0))

    names = sorted(declared, key=lambda name: (-len(callers[name]), name))
    uncalled = [name for name in names if not callers[name]]

    if show:
        for name in names:
            print("%-36s %3d caller(s)  %s" % (name, len(callers[name]),
                                               " ".join(sorted(declared[name]))))

            for site in sorted(callers[name]):
                print("    called by %s" % site)

            if touches[name]:
                print("    touches   %s" % " ".join(sorted(touches[name])))

    print("%d function name(s) that are placeholders: %d called somewhere, %d never called"
          % (len(names), len(names) - len(uncalled), len(uncalled)))

    if ceiling is not None and len(names) > ceiling:
        print("more than the ceiling of %d; name the new one for what it does" % ceiling)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
