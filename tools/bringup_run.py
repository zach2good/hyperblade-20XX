"""Build the bring-up copy, run it for a fixed number of frames, and report what it did.

The output is the trace log and the snapshots, both written by the game process itself.

    python tools/bringup_run.py                 # 90 frames, three seconds at the paced rate
    python tools/bringup_run.py --frames 300
    python tools/bringup_run.py --no-build      # reuse the last bring-up build

    python tools/bringup_run.py --walk 3,4,16,14 --walk-frames 40 --hold 0 \
                                --frames 1500 --trace-pos 120

To reach a match, use --walk: the state that reads the three drome selections does not fill them
in, so a jump straight to it gets an empty match. --trace-pos dumps every object's position every
N frames.

Build needs an x86 developer environment, because it runs `cmake --build build`. It never captures the screen; snapshots come from bitmaps the game built, and each is
converted to a PNG beside it. HB_QUIT_AFTER makes the game quit after the given frames, and the
tool kills anything still alive after the timeout.
"""
import argparse
import glob
import os
import shutil
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmp_to_png

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

BUILD = os.path.join(ROOT, "build")
STAGE = os.path.join(BUILD, "run")

OUT = os.path.join(BUILD, "out")

LINKED = os.path.join(OUT, "hyperx.exe")
RUNNABLE = os.path.join(STAGE, "HYPERX_run.exe")

# Asset directories beside the build, junctioned into the run directory, not copied.
ASSETS = ("Data", "DIRECTX", "AVI", "DOS", "ACRODOS")


def run(command, cwd=None, timeout=None):
    return subprocess.run(command, cwd=cwd, timeout=timeout, capture_output=True, text=True)


def build():
    print("building ...")
    result = run(["cmake", "--build", BUILD])

    if result.returncode != 0:
        for stream in (result.stdout, result.stderr):
            for line in (stream or "").splitlines():
                if " error" in line or "LNK" in line:
                    print(line)

        raise SystemExit("build failed")


def newer_than_link():
    """Sources written after the bring-up image was linked.

    A failed build leaves the previous image in place, and a run of that image would report on
    code that was never built.
    """
    if not os.path.exists(LINKED):
        return []

    linked = os.path.getmtime(LINKED)
    late = []

    for root, dirs, names in os.walk(os.path.join(ROOT, "src")):
        for name in names:
            if not name.endswith((".c", ".cpp", ".h")):
                continue

            path = os.path.join(root, name)

            if os.path.getmtime(path) > linked:
                late.append(path)

    return late


# `Data` in the run directory is a junction into the shared asset tree, so options the game writes
# to `booie.ini` persist. A run snapshots the file and restores it afterwards.
OPTIONS_FILE = os.path.join(STAGE, "Data", "booie.ini")

# The game also writes save files and other data there. The tree is snapshotted by name, size and
# time, and any other change is reported rather than restored.
DATA_DIR = os.path.join(STAGE, "Data")


def same(left, right):
    """Whether two paths name one file, the way this filesystem means it."""
    return os.path.normcase(os.path.abspath(left)) == os.path.normcase(os.path.abspath(right))


def data_snapshot():
    """Every file under the game's data directory, by path, as (size, mtime)."""
    if not os.path.isdir(DATA_DIR):
        return None

    found = {}

    for here, unused_dirs, names in os.walk(DATA_DIR):
        for name in names:
            path = os.path.join(here, name)

            try:
                stat = os.stat(path)
            except OSError:
                continue

            found[path] = (stat.st_size, stat.st_mtime)

    return found


def data_report(before):
    """Say what the run changed under the data directory, apart from the options file."""
    if before is None:
        return []

    after = data_snapshot()

    if after is None:
        return []

    changed = []

    for path in sorted(after):
        # The file is `BOOIE.INI` on disk, so the comparison must ignore case.
        if same(path, OPTIONS_FILE):
            continue

        if path not in before:
            changed.append("created %s" % os.path.relpath(path, DATA_DIR))
        elif after[path] != before[path]:
            changed.append("wrote %s" % os.path.relpath(path, DATA_DIR))

    for path in sorted(before):
        if path not in after:
            changed.append("deleted %s" % os.path.relpath(path, DATA_DIR))

    return changed


def options_snapshot():
    """The options file's bytes before a run, or None when there is no file to keep."""
    if not os.path.exists(OPTIONS_FILE):
        return None

    return open(OPTIONS_FILE, "rb").read()


def options_restore(before):
    """Put the options file back, and say so when the run had changed it."""
    if before is None:
        return False

    if not os.path.exists(OPTIONS_FILE):
        return False

    if open(OPTIONS_FILE, "rb").read() == before:
        return False

    open(OPTIONS_FILE, "wb").write(before)

    return True


def holding(name):
    """The PIDs of every running copy of this checkout's image, by full path.

    Matching by path, not name, leaves bring-up runs from other checkouts alone.
    """
    listing = run(["powershell", "-NoProfile", "-Command",
                   "Get-Process -ErrorAction SilentlyContinue | ForEach-Object { "
                   "if ($_.Path) { $_.Path + '|' + $_.Id } }"])
    wanted = os.path.normcase(os.path.abspath(RUNNABLE))
    pids = []

    for line in (listing.stdout or "").splitlines():
        if "|" not in line:
            continue

        path, pid = line.rsplit("|", 1)

        if os.path.normcase(os.path.abspath(path.strip())) == wanted and pid.strip().isdigit():
            pids.append(int(pid))

    return pids


def launch(environment, timeout):
    """Run this checkout's image and return its exit code, or None after killing it on a timeout.

    It then waits for the image to leave the task list, because the child handle can exit
    while a relaunched process is still running and writing the log.
    """
    child = subprocess.Popen([RUNNABLE], cwd=STAGE, env=environment,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    try:
        child.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        child.kill()
        child.communicate()
        settle(timeout)

        return None

    code = child.returncode

    settle(timeout)

    return code


# The image relaunches itself: the first process exits with 1 after a few seconds and a second
# one does the run. The gap between them varies, so the wait needs a continuous quiet period,
# and any process appearing restarts it.
SETTLE_QUIET_SECONDS = 8.0


def settle(timeout):
    """Wait until this checkout's image has been absent for a continuous stretch."""
    name = os.path.basename(RUNNABLE)
    waited = 0.0
    quiet = 0.0

    while waited < timeout:
        if holding(name):
            quiet = 0.0
        else:
            quiet = quiet + 0.5

            if quiet >= SETTLE_QUIET_SECONDS:
                return

        time.sleep(0.5)
        waited = waited + 0.5

    print("the image is still running after %ds; the log below is incomplete" % timeout)


# Back-to-back runs need about fifteen seconds between them. Runs started closer together die in
# the menus with no crash record. The cause is unknown.
SETTLE_SECONDS = 15


def settleBetweenRuns():
    """Waits long enough after the previous run that the next one is not the previous one's
    teardown. The first call in a process returns at once, because there is nothing to wait
    for."""
    global _played

    if not _played:
        _played = True

        return

    time.sleep(SETTLE_SECONDS)


_played = False

def stage():
    os.makedirs(STAGE, exist_ok=True)

    late = newer_than_link()

    if late:
        for path in sorted(late)[:8]:
            print("newer than the link: %s" % os.path.relpath(path, ROOT))

        raise SystemExit("STALE BRING-UP IMAGE, %d source(s) newer than the link. Build it "
                         "and run again; a result from this image is not a measurement."
                         % len(late))

    # A leftover process holds the image, so the copy below would fail, and it may still be
    # writing the log. The test uses the task list because file probes on a running image
    # succeed on Windows.
    name = os.path.basename(RUNNABLE)

    if holding(name):
        print("a previous run from this checkout is still holding %s; killing it" % name)

        for pid in holding(name):
            run(["taskkill", "/F", "/PID", str(pid)])

        # taskkill returns before Windows releases the image.
        for unused in range(20):
            if not holding(name):
                break

            time.sleep(0.5)

    # A copy under its own name, so the task list can tell this run from any other.
    shutil.copyfile(LINKED, RUNNABLE)

    for name in ASSETS:
        source = os.path.join(OUT, name)
        target = os.path.join(STAGE, name)

        if os.path.isdir(source) and not os.path.exists(target):
            run(["cmd", "/c", "mklink", "/J", target, source])

    for pattern in ("snap_*.bmp", "snap_*.png"):
        for name in glob.glob(os.path.join(STAGE, pattern)):
            os.remove(name)

    log = os.path.join(STAGE, "bringup.log")

    # Keep the previous run's log, which may be from a hand-played session.
    if os.path.exists(log):
        keep = os.path.join(STAGE, "bringup_previous.log")

        if os.path.exists(keep):
            os.remove(keep)

        os.rename(log, keep)


RUN_BANNER = "bringupRunStart"

# A token this run stamps into its own log, so a log written by somebody else's run can be
# told apart from this one's. The image relaunches in a chain and a descendant of an older
# run can write into a log a newer run has just created.
RUN_TAG = "%d_%d" % (os.getpid(), int(time.time() * 1000) % 1000000)


def read_log(path=None):
    """The log, cut down to the last run in it.

    The game appends to `bringup.log`, so a harness that stages once and plays several times
    finds every run in it. Read the log through here, not open().
    """
    if path is None:
        path = os.path.join(STAGE, "bringup.log")

    if not os.path.exists(path):
        return ""

    text = open(path, encoding="utf-8", errors="replace").read()
    cut = text.rfind(RUN_BANNER)

    if cut < 0:
        return text

    return text[cut:]


def report(frames):
    log = os.path.join(STAGE, "bringup.log")

    if not os.path.exists(log):
        print("\nNo bringup.log. The process wrote nothing, so it did not reach the first trace.")
        return 1

    text = read_log(log)

    # The log has to be this run's. See RUN_TAG.
    if ("runTag = " + RUN_TAG) not in text:
        found = [line for line in text.splitlines() if line.startswith("runTag = ")]
        print("\nTHIS LOG IS NOT THIS RUN'S. Wanted runTag %s, found %s."
              % (RUN_TAG, found[-1].split(" = ")[-1] if found else "no tag at all"))
        print("An older run's descendant wrote it. Nothing below can be trusted.")

    lines = [line.rstrip() for line in text.splitlines()]

    print("\n%d trace lines. Last twenty:" % len(lines))

    for line in lines[-20:]:
        print("   ", line)

    snaps = sorted(glob.glob(os.path.join(STAGE, "snap_*.bmp")))

    print("\n%d snapshots:" % len(snaps))

    for name in snaps:
        png = name.rsplit(".", 1)[0] + ".png"

        # A killed process can leave a bitmap half written.
        try:
            size = bmp_to_png.convert(name, png)
        except Exception as reason:
            print("    %s  unreadable: %s" % (os.path.relpath(name, ROOT), reason))

            continue

        print("    %s  %dx%d" % (os.path.relpath(png, ROOT), size[0], size[1]))

    if not snaps:
        print("    none, so nothing reached a bitmap the game had built")

    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--frames", type=int, default=90,
                        help="frames before the game posts its own quit (default 90)")
    parser.add_argument("--timeout", type=int, default=60,
                        help="seconds before this kills it anyway (default 60)")
    parser.add_argument("--state", type=int, default=None,
                        help="engine state to start in, as the splash dialog's success path "
                             "would set it (3 is the first menu)")
    parser.add_argument("--hold", type=int, default=None,
                        help="milliseconds to hold each splash frame (0 skips the splash hold)")
    parser.add_argument("--snap-max", type=int, default=None,
                        help="maximum snapshots to save")
    parser.add_argument("--route", default=None,
                        help="what a screen-building state reads, as "
                             "prev,playerMode,arena[,difficulty]; -2 leaves a word alone. "
                             "State 16 routes to the final ceremony on 9,1,-1 and to the "
                             "training end on 9,1,2,0 or 9,1,8,1")
    parser.add_argument("--fatal-stops", action="store_true",
                        help="end the run at the first fatal the game reports, so the log ends "
                             "with the fatal rather than with whatever fell over after it")
    parser.add_argument("--fixture", default=None,
                        help="which match to play, as home,away,arena indices into the team "
                             "table (fourteen entries; the default fixture comes from "
                             "Data/GAME.CFG and is arena 1)")
    parser.add_argument("--period-ms", type=int, default=None,
                        help="shorten a match period to this many milliseconds, so a run can "
                             "reach the final whistle and what follows it")
    parser.add_argument("--snap-every", type=int, default=None,
                        help="save one snapshot in every N offered, so a long run's cap is "
                             "spread over the whole run instead of its first seconds")
    parser.add_argument("--walk", default=None,
                        help="engine states to walk through in order, as a comma list "
                             "(3,4,16,14 is splash to menus to match)")
    parser.add_argument("--walk-frames", type=int, default=None,
                        help="frames to hold each walked state (default 90)")
    parser.add_argument("--trace-pos", type=int, default=None,
                        help="dump every object's position and velocity every N frames")
    parser.add_argument("--trace-msg", type=int, default=None,
                        help="follow one scene message kind through its first forty posts")
    parser.add_argument("--trace-obj", default=None,
                        help="name the object whose timers to trace step by step")
    parser.add_argument("--no-build", action="store_true", help="reuse the last bring-up build")
    args = parser.parse_args()

    if not args.no_build:
        build()

    stage()

    environment = dict(os.environ)
    environment["HB_QUIT_AFTER"] = str(args.frames)
    environment["HB_RUN_TAG"] = RUN_TAG

    if args.snap_max is not None:
        environment["HB_SNAP_MAX"] = str(args.snap_max)
    elif args.frames > 30:
        environment["HB_SNAP_MAX"] = str(min(args.frames, 100))

    if args.state is not None:
        environment["HB_START_STATE"] = str(args.state)

    if args.hold is not None:
        environment["HB_HOLD_MS"] = str(args.hold)

    if args.walk is not None:
        environment["HB_WALK_STATES"] = args.walk

    if args.walk_frames is not None:
        environment["HB_WALK_FRAMES"] = str(args.walk_frames)

    if args.trace_pos is not None:
        environment["HB_TRACE_POS"] = str(args.trace_pos)

    if args.trace_msg is not None:
        environment["HB_TRACE_MSG"] = str(args.trace_msg)

    if args.trace_obj is not None:
        environment["HB_TRACE_OBJ"] = args.trace_obj

    if args.snap_every is not None:
        environment["HB_SNAP_EVERY"] = str(args.snap_every)

    if args.period_ms is not None:
        environment["HB_PERIOD_MS"] = str(args.period_ms)

    if args.fixture is not None:
        environment["HB_FIXTURE"] = args.fixture

    if args.fatal_stops:
        environment["HB_FATAL_STOPS"] = "1"

    if args.route is not None:
        environment["HB_ROUTE"] = args.route


    # The tag lets a caller outside this process tell this run's log from an older run's.
    print("running %s for %d frames, runTag %s ..."
          % (os.path.relpath(RUNNABLE, ROOT), args.frames, RUN_TAG))

    started = time.time()
    options = options_snapshot()
    data = data_snapshot()

    try:
        code = launch(environment, args.timeout)

        if code is None:
            print("still running after %ds, killed" % args.timeout)
        else:
            print("exited with %d after %.1fs" % (code, time.time() - started))
    except OSError as error:
        print("could not start the run: %s" % error)

    if options_restore(options):
        print("the run changed the game's options file; put back")

    for line in data_report(data):
        print("THE RUN CHANGED THE GAME'S DATA: %s" % line)

    return report(args.frames)


if __name__ == "__main__":
    sys.exit(main())
