"""Walk to every menu state and fail when a screen stops drawing.

`bringup_check.py` says whether a state boots and `check_match.py` says whether a match plays.
Neither sees a screen that constructs and then paints nothing. This walks to each state, takes
the last menu snapshot the run wrote, and counts the pixels that are not black. The floors live
in tools/screen_floors.json, which describes your build and install and so is git-ignored.
--save writes it.

    python tools/check_screens.py             # every state, fail on one that darkened
    python tools/check_screens.py --save      # rewrite the floors from a fresh sweep
    python tools/check_screens.py --states 5,13
    python tools/check_screens.py --frames 300

A state fails when it draws more than MARGIN points below its floor. It exits 1 on any fault.

The measure is for menus only. An undrawn 3D back buffer is one flat colour and scores near 100
per cent, so `check_match.py` checks in-game frames by colour variety through `variety()`.

State 23, the multiplayer setup screen, draws about 3 per cent because a bring-up run has no
network session. That is expected.
"""
import argparse
import json
import os
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bringup_run

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FLOORS = os.path.join(ROOT, "tools", "screen_floors.json")

# The states that build a screen worth looking at. State 14 and 15 are the match, 16 is the
# drome screen the walk passes through, and the rest are menus.
STATES = (3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 18, 19, 20, 21, 23, 25, 27)

# How far below its floor a state may drift before it is a fault. Menus are deterministic, and
# the widgets are a small share of the pixels, so the margin must stay tight.
MARGIN = 1.0

# Anything brighter than this in total is a lit pixel. The menus are dark and a floor of nought
# would count the compression of black as content.
LIT = 30


def read(path):
    """One 24-bit BMP as (bytes, first pixel offset, width, height, row stride)."""
    data = open(path, "rb").read()

    if data[:2] != b"BM":
        raise SystemExit("%s is not a BMP" % path)

    offset = struct.unpack_from("<I", data, 10)[0]
    width = struct.unpack_from("<i", data, 18)[0]
    height = abs(struct.unpack_from("<i", data, 22)[0])
    stride = ((width * 3 + 3) // 4) * 4

    return data, offset, width, height, stride


def share(path):
    """The percentage of a 24-bit BMP's pixels that are not black."""
    data, offset, width, height, stride = read(path)

    lit = 0

    for row in range(height):
        base = offset + row * stride

        for column in range(width):
            at = base + column * 3

            if data[at] + data[at + 1] + data[at + 2] > LIT:
                lit = lit + 1

    return 100.0 * lit / (width * height)


def variety(path):
    """How many distinct colours a BMP holds, and what share its commonest one takes.

    `share` cannot tell a drawn scene from a cleared buffer. A flat fill is one colour at 100
    per cent; a rendered arena has many colours with its commonest under half.
    """
    data, offset, width, height, stride = read(path)

    counts = {}

    for row in range(height):
        base = offset + row * stride

        for column in range(width):
            at = base + column * 3
            key = data[at:at + 3]
            counts[key] = counts.get(key, 0) + 1

    commonest = max(counts.values())

    return len(counts), 100.0 * commonest / (width * height)


def play(state, frames):
    """Walk to one state and hand back the share its last menu snapshot drew."""
    environment = dict(os.environ)
    environment["HB_QUIT_AFTER"] = str(frames)
    environment["HB_WALK_STATES"] = "3,4,16,%d" % state
    environment["HB_WALK_FRAMES"] = "30"
    environment["HB_HOLD_MS"] = "0"
    environment["HB_SNAP_MAX"] = "60"

    for pattern in ("snap_*.bmp", "snap_*.png"):
        for name in bringup_run.glob.glob(os.path.join(bringup_run.STAGE, pattern)):
            os.remove(name)

    try:
        subprocess.run([bringup_run.RUNNABLE], cwd=bringup_run.STAGE, env=environment,
                       timeout=frames // 10 + 90, capture_output=True, text=True)
    except subprocess.TimeoutExpired:
        bringup_run.run(["taskkill", "/F", "/IM", os.path.basename(bringup_run.RUNNABLE)])

        return None

    shots = sorted(bringup_run.glob.glob(os.path.join(bringup_run.STAGE, "snap_*_menu.bmp")))

    if not shots:
        return None

    return share(shots[-1])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--save", action="store_true", help="rewrite the floors")
    parser.add_argument("--states", default=None, help="a comma list, instead of all of them")
    parser.add_argument("--frames", type=int, default=300, help="frames per state")
    args = parser.parse_args()

    wanted = STATES

    if args.states is not None:
        wanted = tuple(int(part) for part in args.states.split(","))

    floors = {}

    if os.path.exists(FLOORS):
        floors = json.load(open(FLOORS, encoding="utf-8"))["states"]
    elif not args.save:
        print("no %s; run with --save on a build you have judged good"
              % os.path.relpath(FLOORS, ROOT))
        return 2

    bringup_run.stage()

    faults = 0
    fresh = {}

    for state in wanted:
        drawn = play(state, args.frames)

        if drawn is None:
            print("FAULT    state %-3d wrote no menu snapshot at all" % state)
            faults = faults + 1

            continue

        fresh[str(state)] = round(drawn, 1)

        if args.save:
            print("  state %-3d draws %5.1f%%" % (state, drawn))

            continue

        floor = floors.get(str(state))

        if floor is None:
            print("UNKNOWN  state %-3d draws %5.1f%%, the floors have never seen it" % (state, drawn))
            faults = faults + 1
        elif drawn < floor - MARGIN:
            print("DARKENED state %-3d draws %5.1f%%, was %5.1f%%" % (state, drawn, floor))
            faults = faults + 1
        else:
            print("  state %-3d draws %5.1f%%, floor %5.1f%%" % (state, drawn, floor))

    if args.save:
        keep = dict(floors)
        keep.update(fresh)

        with open(FLOORS, "w", encoding="utf-8") as handle:
            json.dump({"comment": "Share of non-black pixels each state's screen drew, from "
                                  "tools/check_screens.py --save. A state may drift by the "
                                  "tool's margin; a screen that stops drawing loses tens of "
                                  "points.",
                       "states": keep}, handle, indent=1, sort_keys=True)

        print("floors written for %d state(s)" % len(fresh))

        return 0

    print("%d state(s), %d fault(s)" % (len(wanted), faults))

    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
