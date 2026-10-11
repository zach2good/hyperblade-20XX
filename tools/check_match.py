"""Play one match and fail when the things a match is supposed to do stop happening.

`tools/bringup_check.py` only says whether each engine state boots. This plays a match in the
bring-up build and asserts what a working one produces:

    the run reaches its own frame budget with no crash and no fatal
    both goalies start on the goal mouths, which are at y = +/- 184.5
    the Rok leaves the start gun, which holds it at (0, 0, -95)
    the players move, so the simulation is not frozen
    the camera is placed, moves, and stays inside the arena
    the in-game view draws something
    the plan layer sends messages and the period ends

A match is not deterministic, so each check tests a shape rather than an exact number.

    python tools/check_match.py            # one match, fail on any assertion
    python tools/check_match.py --frames 2400 --verbose
    python tools/check_match.py --full-match   # all three periods, and the match ending

It reuses `tools/bringup_run.py`, so it builds nothing by default and expects a bring-up build
to be present. `--build` asks for one first. Exit code 1 means at least one fault.
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bringup_run
import check_screens

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# bringup_run.read_log cuts the log down to the last run, since the bring-up opens it for append.
LOG = os.path.join(bringup_run.STAGE, "bringup.log")

# The goal mouths, which Physics__InitConstants puts at +/- (semiB - 3) with semiB 187.5.
GOAL_MOUTH_Y = 184.5
GOAL_MOUTH_TOLERANCE = 2.0

# Where the start gun holds the Rok. Anything else means it launched.
GUN_Z = -95.0
LAUNCH_DISTANCE = 5.0

# The state the engine reaches when a period ends.
JUMBOTRON = 17

# The camera is not in the engine's object table, so it has its own checks. A healthy camera
# stays well inside this; a bad transform or uninitialised memory leaves it by orders of magnitude.
CAMERA_LIMIT = 500.0

# A camera that never moves this far over a whole period is frozen. The camera tracks the local
# player, which stands still with no keys held, so this is asserted only under HB_HOLD_KEYS.
CAMERA_TRAVEL = 5.0

CAMERA = re.compile(r"^sceneCamera = \(([-0-9.]+), ([-0-9.]+), ([-0-9.]+)\)")

# The in-game view is measured by colour variety, since the undrawn back buffer is one flat
# colour. Healthy arenas read 52 to 82 colours with the commonest at 43 to 67 per cent; a renderer
# that draws nothing reads about 9 colours at 95 per cent. The colour count is the stronger signal.
FRAME_COLOURS = 12
FRAME_FLAT = 80.0

# Enough in-match frames to be sure one was caught after the arena came up.
FRAME_COUNT = 3

VEC = re.compile(r"^scenePos = \(([-0-9.]+), ([-0-9.]+), ([-0-9.]+)\)")
NAME = re.compile(r"^scenePosName = \[(.*)\]")
SLOT = re.compile(r"^sceneSlot = (-?\d+)")

# The local player does not move unless something drives it, so its travel is asserted only when
# HB_HOLD_KEYS is set.
LOCAL_SLOT = 0
LOCAL_TRAVEL = 20.0

# A short period can end with no goal, so scoring is asserted only on a period at least this long.
SCORING_PERIOD_MS = 60000

SCORE = re.compile(r"^sceneScore([12]) = (\d+)")

# Where an object may be: roughly double the furthest any real object goes.
ARENA_LIMIT = 400.0

# Where a substituted or injured player is parked: `SIDELN.PBN`'s `InjSubstitute` behaviour sets
# the location to (-900, -900, -900). Matched by proximity, because the physics tick moves the
# position slightly after the plan sets it.
PARKED = (-900.0, -900.0, -900.0)
PARKED_TOLERANCE = 20.0


def parked(point, tolerance=None):
    """Whether a position is the corner a substituted or injured player is put in."""
    if tolerance is None:
        tolerance = PARKED_TOLERANCE

    return (abs(point[0] - PARKED[0]) <= tolerance
            and abs(point[1] - PARKED[1]) <= tolerance
            and abs(point[2] - PARKED[2]) <= tolerance)


# A camera following a substituted player goes to the corner with it, offset by the tracking
# distance. The wider tolerance applies only on a run that sent id_MSG_SubstituteDromer.
PARKED_CAMERA_TOLERANCE = 250.0

# A leak across match restarts (HB_LOOP_MATCH) shows as the object count at the end of the run
# exceeding the count at the start.
LEAK_TOLERANCE = 4

# The bring-up counts every message the plan send actions put out. The input device's plan
# dispatches key presses to id_MSG_PassTo, id_MSG_Attack and id_MSG_SubstituteDromer. The kinds
# floor catches a dead plan layer; the attack floor catches an input path that stops driving the
# game. Passes and substitutions vary too much to be floors, so they are only printed.
PLAN_MESSAGE = re.compile(r"^planMsgCount(\d+) = (\d+)")

MSG_PASS_TO = 1
MSG_PASS_RECEIVED = 21
MSG_ATTACK = 25
MSG_SUBSTITUTE = 87

# A quarter of the lowest attack count seen in a healthy run.
ATTACK_LEAST = 15

# Half the number of distinct message kinds a healthy match sends.
MESSAGE_KINDS_LEAST = 20

MESSAGE_KINDS = re.compile(r"^planMsgKinds = (\d+)")

# An object placed at the exact arena centre, where the ellipsoid normal is undefined and
# `CollisionEllipsoid::NormalAtPoint` returns null. `PlanAction__SetLocation` dereferences it
# without a check, as the client does, so this is an access violation. The trace records the
# condition even on a run that survives it.
NULL_NORMAL = re.compile(r"^setPosNormal = 0 ")

# A full match. A period ends at its length plus a run-on, which HB_OVERTIME_MS shortens under the
# bring-up so that all three periods fit in a few thousand frames. The period counter reaching four
# is what `gameengine.cpp` calls the match being over.
PERIOD_NUMBER = re.compile(r"^periodNumber = (\d+)")

FULL_MATCH_PERIOD_MS = 4000
FULL_MATCH_OVERTIME_MS = 1000
FULL_MATCH_FRAMES = 9000

# gameengine.cpp: the counter reaching PERIOD_COUNT is the match ending.
PERIOD_COUNT = 4


def period_faults(path, verbose):
    """The period counter has to walk 1, 2, 3, 4 with no repeat and no gap."""
    seen = []

    for line in bringup_run.read_log(path).splitlines():
        hit = PERIOD_NUMBER.match(line)

        if hit is not None:
            seen.append(int(hit.group(1)))

    if not seen:
        return ["no period trace in the log; the bring-up build is out of date"]

    wanted = list(range(1, PERIOD_COUNT + 1))

    # Only the first four. HB_LOOP_MATCH does not reset the period, so the counter keeps climbing
    # past four as the harness loops.
    front = seen[:PERIOD_COUNT]

    if front == wanted:
        if verbose:
            print("    the periods ran %s and the match reached its end"
                  % " ".join("%d" % n for n in front))

        return []

    return ["the periods ran %s, wanted the first four to be %s"
            % (" ".join("%d" % n for n in seen[:8]), " ".join("%d" % n for n in wanted))]


def plan_messages(path):
    """Every message the plan layer sent, by id, and how many distinct kinds it sent."""
    counts = {}
    kinds = 0

    for line in bringup_run.read_log(path).splitlines():
        hit = PLAN_MESSAGE.match(line)

        if hit is not None:
            counts[int(hit.group(1))] = int(hit.group(2))
            continue

        hit = MESSAGE_KINDS.match(line)

        if hit is not None:
            kinds = int(hit.group(1))

    return counts, kinds


def message_names():
    """ID_MSG out of the developers' own header, read at run time and never copied here."""
    path = os.environ.get("HB_ENUMIDS", os.path.join(ROOT, "build", "out", "Data", "ENUMIDS.H"))

    if not os.path.exists(path):
        return []

    text = open(path, encoding="latin-1").read()
    text = re.sub(r"//.*", "", text)
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    found = re.search(r"enum\s+ID_MSG\s*\{(.*?)\}", text, re.S)

    if found is None:
        return []

    return re.findall(r"(id_\w+)", found.group(1))


def verb_faults(path, verbose):
    """Every fault the plan layer can show, as a list of sentences."""
    counts, kinds = plan_messages(path)
    names = message_names()
    faults = []

    def named(which):
        if which < len(names):
            return "%d %s" % (which, names[which])

        return "%d" % which

    if not counts:
        # The tally prints when the run ends or on WM_DESTROY, so an empty tally means the
        # process died first.
        faults.append("no plan message tally in the log; the run ended before the harness could print it")
        return faults

    if kinds < MESSAGE_KINDS_LEAST:
        faults.append("the plan layer sent %d kinds of message, wanted at least %d; it looks "
                      "stalled" % (kinds, MESSAGE_KINDS_LEAST))

    if counts.get(MSG_ATTACK, 0) < ATTACK_LEAST:
        faults.append("%d attack(s) in a whole match, wanted at least %d; nothing is checking "
                      "anybody" % (counts.get(MSG_ATTACK, 0), ATTACK_LEAST))

    if verbose:
        for which in (MSG_PASS_TO, MSG_PASS_RECEIVED, MSG_ATTACK, MSG_SUBSTITUTE):
            print("    %-28s %d" % (named(which), counts.get(which, 0)))

        print("    %d kinds of plan message in all" % kinds)

    return faults


def local_name(path):
    """The name of whatever object sits in the local player's slot."""
    slot = None

    for line in bringup_run.read_log(path).splitlines():
        hit = SLOT.match(line)

        if hit is not None:
            slot = int(hit.group(1))
            continue

        hit = NAME.match(line)

        if hit is not None and slot == LOCAL_SLOT:
            return hit.group(1)

    return None


def scene(path):
    """Every position dump in the log, as a list of {name: (x, y, z)} one per sample."""
    samples = []
    current = None
    named = None

    for line in bringup_run.read_log(path).splitlines():
        if line.startswith("sceneFrame"):
            current = {}
            samples.append(current)
            continue

        hit = NAME.match(line)

        if hit is not None:
            named = hit.group(1)
            continue

        hit = VEC.match(line)

        if hit is not None and current is not None and named is not None:
            current[named] = (float(hit.group(1)), float(hit.group(2)), float(hit.group(3)))

    return samples


def score_faults(path, verbose):
    """A scoreboard only ever goes up, so a fall means a reset or a stray write landing on it."""
    seen = {"1": [], "2": []}

    for line in bringup_run.read_log(path).splitlines():
        hit = SCORE.match(line)

        if hit is not None:
            seen[hit.group(1)].append(int(hit.group(2)))

    faults = []
    rises = 0

    for side in ("1", "2"):
        run = seen[side]

        for i in range(1, len(run)):
            if run[i] > run[i - 1]:
                rises = rises + 1
                continue

            if run[i] < run[i - 1]:
                faults.append("side %s scored %d and then %d; a scoreboard does not go down"
                              % (side, run[i - 1], run[i]))
                break

    if verbose and not faults and (seen["1"] or seen["2"]):
        print("    the scoreboard rose %d time(s) and fell none" % rises)

    return faults


def best_scores(path):
    """The highest score each side reached."""
    best = {"1": 0, "2": 0}

    for line in bringup_run.read_log(path).splitlines():
        hit = SCORE.match(line)

        if hit is not None:
            best[hit.group(1)] = max(best[hit.group(1)], int(hit.group(2)))

    return best


def camera(path):
    """Every camera position the run traced, in order."""
    seen = []

    for line in bringup_run.read_log(path).splitlines():
        hit = CAMERA.match(line)

        if hit is not None:
            seen.append((float(hit.group(1)), float(hit.group(2)), float(hit.group(3))))

    return seen


def camera_faults(seen, verbose, substituted=False, driven=False):
    """Every fault the camera can show, as a list of sentences."""
    if not seen:
        return ["no camera position in the log; the run needs --trace-pos and a bring-up build "
                "new enough to trace it"]

    faults = []

    stray = [p for p in seen
             if max(abs(p[0]), abs(p[1]), abs(p[2])) > CAMERA_LIMIT
             and not (substituted and parked(p, PARKED_CAMERA_TOLERANCE))]

    if stray:
        faults.append("the camera left the arena by a long way, first at (%.1f, %.1f, %.1f)"
                      % stray[0])

    origin = [p for p in seen
              if abs(p[0]) < 0.01 and abs(p[1]) < 0.01 and abs(p[2]) < 0.01]

    if origin:
        faults.append("the camera sat at the origin for %d of %d samples, which is where it is "
                      "before anything places it" % (len(origin), len(seen)))

    first = seen[0]
    travelled = max(max(abs(p[i] - first[i]) for i in range(3)) for p in seen)

    if driven and travelled < CAMERA_TRAVEL:
        faults.append("the camera moved %.2f units over the whole run with the keys held; it "
                      "looks frozen" % travelled)
    elif verbose and not faults:
        print("    the camera moved %.0f units over %d samples" % (travelled, len(seen)))

    return faults


def frame_faults(verbose):
    """Every fault the in-game view can show, as a list of sentences."""
    shots = sorted(bringup_run.glob.glob(
        os.path.join(bringup_run.STAGE, "snap_*_frame.bmp")))

    if len(shots) < FRAME_COUNT:
        return ["%d in-game frame(s) were captured, wanted %d; the run needs HB_SNAP_EVERY"
                % (len(shots), FRAME_COUNT)]

    colours, commonest = check_screens.variety(shots[-1])

    if colours < FRAME_COLOURS or commonest > FRAME_FLAT:
        return ["the in-game view drew %d colour(s) with its commonest at %.1f per cent, so the "
                "renderer filled the buffer and drew nothing on it" % (colours, commonest)]

    if verbose:
        print("    the view drew %d colours, commonest %.0f%%, over %d frames"
              % (colours, commonest, len(shots)))

    return []


def moved(samples, name, distance):
    """Whether one object ever got that far from where it started."""
    seen = [s[name] for s in samples if name in s]

    if len(seen) < 2:
        return False

    first = seen[0]

    for point in seen[1:]:
        span = max(abs(point[0] - first[0]), abs(point[1] - first[1]),
                   abs(point[2] - first[2]))

        if span >= distance:
            return True

    return False


def check(text, samples, verbose, period_ms=0, full_match=False):
    """Every fault a match can show, as a list of sentences."""
    faults = []

    if "CRASH code" in text:
        first = [line for line in text.splitlines() if line.startswith("CRASH code")][0]
        faults.append("the run crashed: %s" % first)

    if "FATAL_SKIPPED" in text:
        boxes = [line for line in text.splitlines() if line.startswith("errorBox")]
        faults.append("the game raised a fatal: %s"
                      % (boxes[-1] if boxes else "no errorBox line"))

    if not samples:
        faults.append("no position dump in the log; the run needs --trace-pos")

        return faults

    # The goalies. Their names come from the roster, so they are found by where they stand
    # rather than by name: a match has exactly two objects that start on a goal mouth.
    opening = samples[0]
    onmouth = [name for name, point in opening.items()
               if abs(abs(point[1]) - GOAL_MOUTH_Y) <= GOAL_MOUTH_TOLERANCE]

    if len(onmouth) < 2:
        faults.append("%d object(s) start on a goal mouth at y = +/- %.1f, wanted two; the "
                      "arena extent is the first thing to check"
                      % (len(onmouth), GOAL_MOUTH_Y))
    elif verbose:
        print("    goalies on the mouths: %s" % ", ".join(sorted(onmouth)))

    # The Rok. Its name is the drome's, so it is found by where the gun holds it.
    ingun = [name for name, point in opening.items()
             if abs(point[0]) < 1.0 and abs(point[1]) < 1.0 and abs(point[2] - GUN_Z) < 1.0]

    if not ingun:
        faults.append("nothing starts in the start gun at (0, 0, %.0f)" % GUN_Z)
    else:
        rok = ingun[0]

        if not moved(samples, rok, LAUNCH_DISTANCE):
            faults.append("%s never left the start gun; the timer slots on CTimerActivity "
                          "are the first thing to check" % rok)
        elif verbose:
            print("    the Rok is %s and it launched" % rok)

    # The skaters. Anything that moves a long way and is not the Rok will do: a frozen
    # simulation is the shape to catch, not a particular player.
    movers = [name for name in opening
              if name not in ingun and moved(samples, name, 20.0)]

    if len(movers) < 2:
        faults.append("%d object(s) moved more than twenty units; the simulation looks frozen"
                      % len(movers))
    elif verbose:
        print("    %d objects moved: %s" % (len(movers), ", ".join(sorted(movers)[:6])))

    # Only when something is driving it. See LOCAL_SLOT above.
    if os.environ.get("HB_HOLD_KEYS"):
        driven = local_name(LOG)

        if driven is None:
            faults.append("no slot trace in the log, so the local player cannot be found")
        elif not moved(samples, driven, LOCAL_TRAVEL):
            faults.append("%s is the local player and HB_HOLD_KEYS is set, but it never moved "
                          "%.0f units; the injection is firing and not holding"
                          % (driven, LOCAL_TRAVEL))
        elif verbose:
            print("    the held keys drove %s, the local player" % driven)

    if period_ms >= SCORING_PERIOD_MS:
        best = best_scores(LOG)

        if best["1"] + best["2"] == 0:
            faults.append("no goal in a %d ms period; the scoring path is the thing to check"
                          % period_ms)
        elif verbose:
            print("    the match finished %s-%s" % (best["1"], best["2"]))

    faults.extend(verb_faults(LOG, verbose))
    faults.extend(score_faults(LOG, verbose))

    if full_match:
        faults.extend(period_faults(LOG, verbose))

    empty = [line for line in text.splitlines() if NULL_NORMAL.match(line)]

    if empty:
        faults.append("%d object(s) were placed at the arena centre, where the ellipsoid normal "
                      "is undefined and SetLocation reads it anyway; this is the access "
                      "violation" % len(empty))

    if len(samples) >= 8:
        quarter = max(len(samples) // 4, 1)
        first = sum(len(s) for s in samples[:quarter]) / float(quarter)
        last = sum(len(s) for s in samples[-quarter:]) / float(quarter)

        if last - first > LEAK_TOLERANCE:
            faults.append("the object count grew from %.1f to %.1f over the run; a match "
                          "restart is leaving objects behind" % (first, last))
        elif verbose:
            print("    the object count held at %.0f" % last)

    stray = []
    substituted = plan_messages(LOG)[0].get(MSG_SUBSTITUTE, 0) > 0

    for sample in samples:
        for name, point in sample.items():
            if parked(point):
                continue

            if substituted and parked(point, PARKED_CAMERA_TOLERANCE):
                continue

            if max(abs(point[0]), abs(point[1]), abs(point[2])) > ARENA_LIMIT:
                stray.append((name, point))

    if stray:
        faults.append("%s reached (%.0f, %.0f, %.0f), outside the arena by a long way; %d "
                      "sample(s) are out" % (stray[0][0], stray[0][1][0], stray[0][1][1],
                                             stray[0][1][2], len(stray)))
    elif verbose:
        print("    every object stayed inside the arena")

    counts, unused_kinds = plan_messages(LOG)
    faults = faults + camera_faults(camera(LOG), verbose,
                                    counts.get(MSG_SUBSTITUTE, 0) > 0,
                                    bool(os.environ.get("HB_HOLD_KEYS")))
    faults = faults + frame_faults(verbose)

    # HB_LOOP_MATCH leaves the jumbotron in the same frame it enters, so the state trace can miss
    # it. A restart is the same evidence that the period ended.
    restarts = len(re.findall(r"^loopMatchRestart", text, re.M))

    if "engineState = %d " % JUMBOTRON not in text and restarts == 0:
        faults.append("the period never ended; the engine did not reach state %d" % JUMBOTRON)
    elif verbose and restarts:
        print("    the period ended %d time(s) and the match restarted" % restarts)

    return faults


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--frames", type=int, default=2400, help="frames to play (default 2400)")
    parser.add_argument("--period-ms", type=int, default=18000,
                        help="period length, short enough to reach the whistle")
    parser.add_argument("--fixture", default=None, help="home,away,arena")
    parser.add_argument("--build", action="store_true", help="build the bring-up tree first")
    parser.add_argument("--verbose", action="store_true", help="say what each check found")
    parser.add_argument("--full-match", action="store_true",
                        help="play all three periods with a short run-on and assert the "
                             "period counter walks 1, 2, 3, 4")
    args = parser.parse_args()

    if args.build:
        bringup_run.build()

    bringup_run.stage()

    environment = dict(os.environ)
    environment["HB_QUIT_AFTER"] = str(args.frames)
    environment["HB_WALK_STATES"] = "3,4,16,14"
    environment["HB_WALK_FRAMES"] = "40"
    environment["HB_HOLD_MS"] = "0"
    environment["HB_TRACE_POS"] = "150"
    environment["HB_PERIOD_MS"] = str(args.period_ms)

    if args.full_match:
        environment["HB_PERIOD_MS"] = str(FULL_MATCH_PERIOD_MS)
        environment["HB_OVERTIME_MS"] = str(FULL_MATCH_OVERTIME_MS)
        environment["HB_LOOP_MATCH"] = "1"

        args.frames = max(args.frames, FULL_MATCH_FRAMES)
        environment["HB_QUIT_AFTER"] = str(args.frames)
    environment["HB_SNAP_MAX"] = "12"
    environment["HB_SNAP_EVERY"] = "60"

    if args.fixture is not None:
        environment["HB_FIXTURE"] = args.fixture

    print("playing %d frames ..." % args.frames)

    # The staged Data is a junction into the shared asset tree, so options the game writes persist.
    options = bringup_run.options_snapshot()

    if bringup_run.launch(environment, args.frames // 10 + 120) is None:
        print("the run had to be killed, which is a fault in itself")
        bringup_run.options_restore(options)

        return 1

    bringup_run.options_restore(options)

    if not os.path.exists(LOG):
        print("no bringup.log; the process wrote nothing")

        return 1

    text = bringup_run.read_log(LOG)
    faults = check(text, scene(LOG), args.verbose, args.period_ms, args.full_match)

    for fault in faults:
        print("FAULT  %s" % fault)

    print("%d fault(s)" % len(faults))

    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
