"""Play every arena and fail when one of them stops drawing its scene.

`check_match.py` plays one fixture. An arena whose geometry, palette or texture set fails to
load draws a flat buffer and passes every other check, so this plays each arena in turn and
tests its last in-game frame, and reports what the plan layer did in each run.

    python tools/check_arenas.py                 # all fourteen
    python tools/check_arenas.py --arenas 3,9
    python tools/check_arenas.py --frames 1500

The frame floors are imported from `check_match` so the two cannot drift apart.

Teams are held at 5 and 10 and only the arena moves: selecting Nemesis fails for a reason
unrelated to the arena, which tools/check_model_files.py watches.

KNOWN names an arena this disc cannot draw. An arena that is flat and not named fails, and an
arena that is named and draws fails too, so a fix cannot leave a stale entry behind.
"""
import argparse
import glob
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bringup_run
import check_match
import check_screens

ARENA_COUNT = 14

LOG = os.path.join(bringup_run.STAGE, "bringup.log")

# The plan floors, set at about a third of the quietest arena over a 1200 frame sweep and far
# above a dead plan layer. They are separate from check_match's, which are set at 2400 frames.
# Passes are reported and never asserted, since the AI rarely passes on its own. A match is not
# deterministic, so one arena creeping toward a floor is noise until a second sweep agrees.
ARENA_ATTACK_LEAST = 10
ARENA_KINDS_LEAST = 20

# The two teams every fixture uses, chosen because both rosters load.
HOME_TEAM = 5
AWAY_TEAM = 10

KNOWN = {}


def verbs(path):
    """What the plan layer did in the run just finished, for the report line."""
    counts, kinds = check_match.plan_messages(path)

    return (counts.get(check_match.MSG_ATTACK, 0), counts.get(check_match.MSG_PASS_TO, 0), kinds)


def play(arena, frames):
    """One arena's last in-game frame, as (colours, commonest share), or None."""
    for path in glob.glob(os.path.join(bringup_run.STAGE, "snap_*.bmp")):
        os.remove(path)

    environment = dict(os.environ)
    environment["HB_QUIT_AFTER"] = str(frames)
    environment["HB_WALK_STATES"] = "3,4,16,14"
    environment["HB_WALK_FRAMES"] = "40"
    environment["HB_HOLD_MS"] = "0"
    environment["HB_PERIOD_MS"] = "18000"
    environment["HB_SNAP_MAX"] = "8"
    environment["HB_SNAP_EVERY"] = "60"
    environment["HB_FIXTURE"] = "%d,%d,%d" % (HOME_TEAM, AWAY_TEAM, arena)

        # The staged Data is a junction into the shared asset tree, so options the game writes
        # persist.
    options = bringup_run.options_snapshot()

    try:
        subprocess.run([bringup_run.RUNNABLE], cwd=bringup_run.STAGE, env=environment,
                       timeout=frames // 10 + 150, capture_output=True, text=True)
    except subprocess.TimeoutExpired:
        bringup_run.run(["taskkill", "/F", "/IM", os.path.basename(bringup_run.RUNNABLE)])
        bringup_run.options_restore(options)

        return None

    bringup_run.options_restore(options)

    shots = sorted(glob.glob(os.path.join(bringup_run.STAGE, "snap_*_frame.bmp")))

    if len(shots) < check_match.FRAME_COUNT:
        return None

    return check_screens.variety(shots[-1])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--arenas", default=None, help="a comma list, instead of all of them")
    parser.add_argument("--frames", type=int, default=900, help="frames per arena")
    parser.add_argument("--build", action="store_true", help="build the bring-up tree first")
    args = parser.parse_args()

    wanted = range(ARENA_COUNT)

    if args.arenas is not None:
        wanted = [int(part) for part in args.arenas.split(",")]

    if args.build:
        bringup_run.build()

    bringup_run.stage()

    faults = 0
    drew = []

    for arena in wanted:
        # Not the previous run's teardown. See bringup_run.settle.
        bringup_run.settleBetweenRuns()

        drawn = play(arena, args.frames)

        if drawn is None:
            print("FAULT    arena %-3d never reached a playable frame" % arena)
            faults = faults + 1

            continue

        colours, commonest = drawn
        flat = colours < check_match.FRAME_COLOURS or commonest > check_match.FRAME_FLAT

        if not flat:
            drew.append(arena)

            if arena in KNOWN:
                print("DRAWS NOW arena %-3d remove its row from KNOWN in this file" % arena)
                faults = faults + 1
            else:
                attacks, passes, kinds = verbs(LOG)
                quiet = ""

                if attacks < ARENA_ATTACK_LEAST or kinds < ARENA_KINDS_LEAST:
                    quiet = "   <== nobody is playing in it"
                    faults = faults + 1

                print("  arena %-3d %3d colours, commonest %5.1f%%, %4d attack(s), %d pass(es), "
                      "%d message kind(s)%s"
                      % (arena, colours, commonest, attacks, passes, kinds, quiet))

            continue

        if arena in KNOWN:
            print("  arena %-3d flat, known: %s" % (arena, KNOWN[arena]))

            continue

        print("FLAT     arena %-3d drew %d colour(s), commonest %.1f%%; it filled the buffer "
              "and drew nothing on it" % (arena, colours, commonest))
        faults = faults + 1

        sys.stdout.flush()

    print("%d arena(s), %d drew, %d fault(s)" % (len(list(wanted)), len(drew), faults))

    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
