"""Run every engine state through the bring-up binary and report the three signals.

Judge each state on its exit code, its CRASH lines and its last trace line. The wild-call fault
arrives in bursts across a sweep and clears on its own, so judge a sweep against a second sweep.

    python tools/bringup_sweep.py                 # states 0 to 28, 60 frames each
    python tools/bringup_sweep.py --states 14,15
    python tools/bringup_sweep.py --tag before    # keep each state's log under build-harness

This does not build. Build with tools/bringup_run.py, or cmake --build build-harness, first.
It refuses to run when a source under src/ or cmake/ is newer than the linked image; pass
--stale-ok to sweep anyway.

It stages: stage() copies build-harness/out/hyperx.exe to build-harness/run/HYPERX_run.exe,
which is what runs. Without the copy, the sweep would run the previous build.
"""
import argparse
import glob
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bringup_run

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

STAGE = os.path.join(ROOT, "build-harness", "run")
RUNNABLE = os.path.join(STAGE, "HYPERX_run.exe")
LOG = os.path.join(STAGE, "bringup.log")

STATE_COUNT = 29


WATCHED = (("src", (".c", ".cpp", ".h")),
           ("cmake", (".cmake",)))


def staleSources(linked):
    """Every watched source modified after the linked image was written."""
    when = os.path.getmtime(linked)
    found = []

    for directory, suffixes in WATCHED:
        where = os.path.join(ROOT, directory)

        if not os.path.isdir(where):
            continue

        for name in os.listdir(where):
            if not name.endswith(suffixes):
                continue

            path = os.path.join(where, name)

            if os.path.getmtime(path) > when:
                found.append(os.path.join(directory, name))

    return sorted(found)


def clean():
    for pattern in ("snap_*.bmp", "snap_*.png"):
        for name in glob.glob(os.path.join(STAGE, pattern)):
            os.remove(name)

    if os.path.exists(LOG):
        os.remove(LOG)


def runOne(state, frames, timeout):
    clean()

    environment = dict(os.environ)
    environment["HB_QUIT_AFTER"] = str(frames)
    environment["HB_START_STATE"] = str(state)
    environment["HB_HOLD_MS"] = "0"

    code = "timeout"

    # Its own child only; see bringup_run.launch.
    returncode = bringup_run.launch(environment, timeout)

    if returncode is not None:
        code = "0x%08x" % (returncode & 0xffffffff)

    lines = []

    if os.path.exists(LOG):
        lines = [line.rstrip() for line in open(LOG, encoding="utf-8", errors="replace")]

    return code, lines


def keep(tag, state, lines):
    if tag is None:
        return

    where = os.path.join(ROOT, "build-harness", "sweeps", tag)

    if not os.path.isdir(where):
        os.makedirs(where)

    handle = open(os.path.join(where, "state_%02d.log" % state), "w", encoding="utf-8")
    handle.write("\n".join(lines))
    handle.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--states", default=None,
                        help="comma-separated states (default every one)")
    parser.add_argument("--frames", type=int, default=60,
                        help="frames before the game posts its own quit (default 60)")
    parser.add_argument("--timeout", type=int, default=60,
                        help="seconds before this kills a state anyway (default 60)")
    parser.add_argument("--tag", default=None,
                        help="keep each state's log under build-harness/sweeps/<tag>")
    parser.add_argument("--stale-ok", action="store_true",
                        help="sweep even where a source is newer than the linked image")
    arguments = parser.parse_args()

    linked = os.path.join(ROOT, "build-harness", "out", "hyperx.exe")

    if not os.path.exists(linked):
        raise SystemExit("no build-harness/out/hyperx.exe; run tools/bringup_run.py first")

    stale = staleSources(linked)

    if stale and not arguments.stale_ok:
        print("%s is older than %d modified source(s):" % (
            os.path.relpath(linked, ROOT), len(stale)))

        for name in stale[:5]:
            print("    " + name)

        raise SystemExit("build first with tools/bringup_run.py, or pass --stale-ok")

    # Never skip this. See the module comment.
    bringup_run.stage()

    states = range(STATE_COUNT)

    if arguments.states is not None:
        states = [int(one) for one in arguments.states.split(",")]

    nClean = 0

    print("state  exit        crash / last trace line")

    for state in states:
        code, lines = runOne(state, arguments.frames, arguments.timeout)

        keep(arguments.tag, state, lines)

        crashes = [line.strip() for line in lines if "CRASH" in line]

        # A run past the game's own fatal measures a program the original refuses to run.
        # src/app/gameapp.cpp turns the fatal into this trace under HYPERX_BRINGUP.
        fatal = [line.strip() for line in lines if "fatalShutdownSkipped" in line]

        if code == "0x00000000" and not crashes:
            nClean += 1

        if fatal and (crashes or code != "0x00000000"):
            reason = [line.strip() for line in lines if "errorBox" in line]
            note = "ABORTED at the game's own fatal"
            if reason:
                note = note + ": " + reason[-1]
        elif crashes:
            note = " | ".join(crashes)
        elif lines:
            note = lines[-1].strip()
        else:
            note = "no log, so the process wrote nothing"

        print("%5d  %-10s  %s" % (state, code, note[:160]))

    print("\n%d of %d exit zero with no CRASH line" % (nClean, len(list(states))))

    return 0


if __name__ == "__main__":
    sys.exit(main())
