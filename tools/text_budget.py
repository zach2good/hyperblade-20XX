"""Compare HYPERX.exe's `.text` with the original's, region by region.

Reads build/out/HYPERX.map for our regions: game, libjpeg, and LIBCMT with the import thunks. On
the original's side a region starts at its lowest address build/report.json pairs, through
HYPERX_sym.map, and `.text$x` starts at the first `Unwind@` in symbols/functions.csv. Also prints
the `.CRT$XCU` entry counts and each original entry no row pairs. Read only.

    python tools/text_budget.py
    python tools/text_budget.py --selftest
"""

import json
import re
import sys
from pathlib import Path

import yaml
from reccmp.formats import detect_image

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "build" / "out"
# Both links put .CRT$XCU just past .CRT$XCA's one zero entry at the head of .data.
XCU_START = 0x004BB004
XCU_LIMIT = 1024

# 0001:00093030       _jpeg_CreateDecompress     00494030 f   jpeg6a:jdapimin.c.obj
FUNCTION_LINE = re.compile(r"^ 0001:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8}) f\s+(?:i\s+)?(\S+)")
SECTION_LINE = re.compile(r"^ 0001:([0-9a-f]{8}) ([0-9a-f]{8})H (\S+)\s+CODE")


def region(owner):
    if owner.startswith("jpeg6a:"):
        return "libjpeg"

    if owner.startswith("LIBCMT:"):
        return "LIBCMT"

    if ":" in owner:
        return "import thunks"

    return "game"


def runs(lines):
    """Answers [region, first address, end address, [(address, symbol)]] for each run of one
    region in `.text`, in address order, and the start of `.text$x`."""
    functions = []
    funclets = None
    text_end = None
    for line in lines:
        section = SECTION_LINE.match(line)
        if section:
            start = 0x401000 + int(section.group(1), 16)
            if section.group(3) == ".text$x":
                funclets = start
            elif section.group(3) == ".text":
                text_end = start + int(section.group(2), 16)
            continue

        function = FUNCTION_LINE.match(line)
        if function:
            functions.append((int(function.group(2), 16), region(function.group(3)), function.group(1)))

    found = []
    for addr, name, symbol in sorted(functions):
        if funclets is not None and addr >= funclets:
            continue

        if found and found[-1][0] == name:
            found[-1][3].append((addr, symbol))
        else:
            found.append([name, addr, None, [(addr, symbol)]])

    for number, run in enumerate(found):
        if number + 1 < len(found):
            run[2] = found[number + 1][1]
        else:
            run[2] = funclets if funclets is not None else text_end

    return found, funclets


def symbol_addresses(lines):
    addresses = {}
    for line in lines:
        function = FUNCTION_LINE.match(line)
        if function:
            addresses.setdefault(function.group(1), int(function.group(2), 16))

    return addresses


def budget(found, symbols, pairs, original_funclets):
    """Answers (region, our size, the original's size) for game, libjpeg, and LIBCMT with the
    import thunks, which the two links order differently. A region starts at its lowest paired
    original address, the game at 0x401000, and ends where the next starts or `.text$x` does."""
    ours = {}
    lowest = {}
    for run in found:
        name = "LIBCMT, imports" if run[0] in ("LIBCMT", "import thunks") else run[0]
        ours[name] = ours.get(name, 0) + run[2] - run[1]
        for addr, symbol in run[3]:
            theirs = pairs.get(symbols.get(symbol))
            if theirs is not None:
                lowest[name] = min(lowest.get(name, theirs), theirs)

    lowest["game"] = 0x401000
    order = sorted(lowest, key=lowest.get)
    bounds = [lowest[name] for name in order] + [original_funclets]
    return [(name, ours[name], bounds[number + 1] - bounds[number]) for number, name in enumerate(order)]


def xcu_entries(image):
    """Answers the `.CRT$XCU` entries from the table's start up to the zero that closes it."""
    data = image.read(XCU_START, XCU_LIMIT * 4)
    entries = []
    for at in range(0, len(data), 4):
        entry = int.from_bytes(data[at:at + 4], "little")
        if entry == 0:
            break

        entries.append(entry)

    return entries


def main():
    lines = (OUT / "HYPERX.map").read_text(errors="replace").splitlines()
    symbols = symbol_addresses((OUT / "HYPERX_sym.map").read_text(errors="replace").splitlines())
    rows = json.loads((ROOT / "build" / "report.json").read_text())["data"]
    pairs = {int(row["recomp"], 16): int(row["address"], 16) for row in rows if row.get("recomp")}
    paired = {int(row["address"], 16) for row in rows if row.get("recomp")}

    found, funclets = runs(lines)
    unwinds = [int(line.split(",")[0], 16) for line in (ROOT / "symbols" / "functions.csv").open()
               if ",Unwind@" in line]

    print("%-16s %10s %10s %8s" % ("region", "ours", "original", "delta"))
    for name, size, theirs in budget(found, symbols, pairs, min(unwinds)):
        print("%-16s %#10x %#10x %+8d" % (name, size, theirs, size - theirs))

    print("%-14s %#10x, the original's at %#x" % (".text$x", funclets, min(unwinds)))

    user = yaml.safe_load((ROOT / "reccmp-user.yml").read_text())
    original = detect_image(user["targets"]["HYPERX"]["path"])
    ours = xcu_entries(detect_image(str(OUT / "HYPERX.exe")))
    theirs = xcu_entries(original)
    print(".CRT$XCU: %d entries, the original's %d" % (len(ours), len(theirs)))
    for index, addr in enumerate(theirs):
        if addr not in paired:
            print("  entry %d, %#x, has no pair" % (index, addr))

    return 0


def selftest():
    """Checks run splitting and region names on a toy map."""
    lines = [
        " 0001:00000000 00000300H .text                   CODE",
        " 0001:00000300 00000010H .text$x                 CODE",
        " 0001:00000000       ?A@@YAXXZ 00401000 f   a.cpp.obj",
        " 0001:00000100       ?B@@YAXXZ 00401100 f i b.cpp.obj",
        " 0001:00000180       _jpeg_x 00401180 f   jpeg6a:j.c.obj",
        " 0001:00000200       _strlen 00401200 f   LIBCMT:strlen.obj",
        " 0001:00000280       _DirectDrawCreate@12 00401280 f   ddraw:DDRAW.dll",
        " 0001:00000300       Unwind@1 00401300 f   a.cpp.obj",
    ]
    found, funclets = runs(lines)
    assert [(run[0], run[1], run[2]) for run in found] == [
        ("game", 0x401000, 0x401180), ("libjpeg", 0x401180, 0x401200),
        ("LIBCMT", 0x401200, 0x401280), ("import thunks", 0x401280, 0x401300)], found
    assert funclets == 0x401300
    # libjpeg's first function pairs at 0x500200 and LIBCMT's at 0x500290; .text$x at 0x500380.
    symbols = {"_jpeg_x": 0x402180, "_strlen": 0x402200}
    pairs = {0x402180: 0x500200, 0x402200: 0x500290}
    assert budget(found, symbols, pairs, 0x500380) == [
        ("game", 0x180, 0xff200), ("libjpeg", 0x80, 0x90), ("LIBCMT, imports", 0x100, 0xf0)]
    print("selftest passed")
    return 0


if __name__ == "__main__":
    if sys.argv[1:] == ["--selftest"]:
        raise SystemExit(selftest())

    raise SystemExit(main())
