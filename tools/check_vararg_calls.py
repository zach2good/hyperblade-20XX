# Report GameApp::Error and GameApp::Fatal calls whose format string wants arguments and gets none.
#
# The decompiler renders a varargs call with its format string only, so a transcribed call can
# push fewer values than the client. The client's `add esp` after the call, divided by four,
# gives the true count; this only finds the calls worth checking.
#
# A format with conversions and no arguments is a fault and exits 1. A call whose argument count
# differs from its conversion count is a warning and does not fail, since source alone cannot
# settle the count.

import glob
import re
import sys

CALL = re.compile(
    r'\.(?:Error|Fatal)\s*\(\s*("(?:[^"\\]|\\.)*")\s*(,|\))',
    re.S,
)

CONVERSION = re.compile(r"%[-0-9.#lh]*[sdiuxXcfgep]")

# Calls where the client itself passes nothing for its %s. Keyed on the file and a substring of
# the format string; the address is for the reader.
ALLOWED = {
    ("src/anim/animscripttable.cpp", "Assembly file"),   # 0x00418210
    ("src/anim/animfile.cpp", "Animation file"),  # 0x00493cb0
}

def top_level_commas(text):
    """Arguments after the format: commas outside brackets and strings."""
    depth = 0
    count = 0
    in_string = False
    escape = False
    for ch in text:
        if escape:
            escape = False
        elif ch == chr(92):
            escape = True
        elif ch == chr(34):
            in_string = not in_string
        elif in_string:
            pass
        elif ch in chr(40) + chr(91):
            depth += 1
        elif ch in chr(41) + chr(93):
            depth -= 1
        elif ch == chr(44) and depth == 0:
            count += 1
    return count


mismatched = []
short = []

for path in sorted(glob.glob("src/**/*.cpp", recursive=True)):
    text = open(path, encoding="utf-8", errors="replace").read()

    for match in CALL.finditer(text):
        wanted = len(CONVERSION.findall(match.group(1)))

        if wanted == 0:
            continue

        if match.group(2) != ")":
            close = text.find(chr(59), match.start())
            if close > 0:
                inner = text[match.end() - 1:close]
                passed = top_level_commas(inner)
                if passed != wanted:
                    mismatched.append((path,
                                       text[: match.start()].count(chr(10)) + 1,
                                       wanted, passed, match.group(1)[:48]))
            continue

        line = text[: match.start()].count("\n") + 1

        here = path.replace("\\", "/")

        if any(here == f and text_key in match.group(1) for f, text_key in ALLOWED):
            continue

        short.append((path, line, wanted, match.group(1)[:58]))

for path, line, wanted, fmt in short:
    print("%s:%d  wants %d, passes none  %s" % (path, line, wanted, fmt))

for path, line, wanted, passed, fmt in mismatched:
    print("WARNING %s:%d  wants %d, passes %d  %s"
          % (path, line, wanted, passed, fmt))

print("%d error calls with conversions and no arguments; %d with a count mismatch"
      % (len(short), len(mismatched)))

sys.exit(1 if short else 0)
