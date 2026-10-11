"""Compares a bring-up sweep against a local baseline and names the states that broke.

    python tools/bringup_check.py             # sweep, compare, exit non-zero on a regression
    python tools/bringup_check.py --save      # rewrite the baseline from a fresh sweep
    python tools/bringup_check.py --states 8,9,11

It reports four transitions: a state that reached the frame budget and now crashes, a state
that crashed and now does not, a state whose crash moved to a different function, and a state
the baseline has never seen. It exits 1 for the first and the last, so "nothing to check" never
reads as a pass.

The baseline, tools/bringup_baseline.json, describes your build and your install, so it is
git-ignored. --save writes it. The baseline records a function and an offset, never an address. The bring-up image is linked
/OPT:REF, so every function moves when a translation unit is added.

A state that newly fails is re-run (--confirm times) and one clean re-run clears it, because
the sweep's wild-call fault comes in bursts that clear on their own. A state marked "flaky" in
the baseline only warns. Mark a state flaky only after seeing it pass and fail on the same
build, and say so in its "note".

This needs a linked bring-up build and the game's assets, so it cannot run on CI. Run it before
a merge or a milestone.
"""
import argparse
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bringup_run
import bringup_sweep

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

BASELINE = os.path.join(ROOT, "tools", "bringup_baseline.json")
LINKED = os.path.join(ROOT, "build-bringup", "out", "HYPERX.exe")
MAP = os.path.join(ROOT, "build-bringup", "out", "HYPERX.map")

IMAGE_BASE = 0x400000

# 0001:0000abcd  ?Foo@@YAXXZ  0040abcd f  file.obj
ENTRY = re.compile(r"^\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+([0-9a-fA-F]{8})\s")

FROM = re.compile(r"CRASH from = (0x[0-9a-fA-F]+) at (esp\+0x[0-9a-fA-F]+)")
CODE = re.compile(r"CRASH code = (0x[0-9a-fA-F]+)")
AT = re.compile(r"CRASH code = 0x[0-9a-fA-F]+ at = (0x[0-9a-fA-F]+)")


def symbols():
    found = []

    for line in open(MAP, encoding="utf-8", errors="replace"):
        hit = ENTRY.match(line)

        if hit is not None:
            found.append((int(hit.group(2), 16), hit.group(1)))

    found.sort()

    return found


def name(table, address):
    best = None

    for start, symbol in table:
        if start > address:
            break

        best = (start, symbol)

    if best is None:
        return "0x%08x" % address

    return "%s + 0x%x" % (best[1], address - best[0])


def observe(table, code, lines):
    """One state's result, in terms that survive the next link.

    The site is the faulting instruction, from the reporter's "at =" word. "under" is the
    innermost stack frame it resolved, with its stack offset. A frame at esp+0 is the faulting
    function's return address; one further up may be a dead word in some other frame.
    """
    # A run that passed the game's own fatal is running a program the original refuses to run.
    # src/app/gameapp.cpp traces the suppressed shutdown.
    fatal = [line for line in lines if "fatalShutdownSkipped" in line]
    crashed = [line for line in lines if "CRASH" in line]

    # Many states pass a suppressed fatal and still reach the frame budget. A state is aborted
    # only when it also died, and then the fatal is the cause.
    if fatal and (crashed or code != "0x00000000"):
        reasons = [line for line in lines if "errorBox" in line]
        answer = {"result": "aborted"}

        if reasons:
            answer["site"] = reasons[-1].strip()[:120]

        return answer

    crashes = [line for line in lines if "CRASH" in line]

    if not crashes:
        if code == "0x00000000":
            return {"result": "clean"}

        return {"result": "exit", "code": code}

    if code == "timeout":
        result = "timeout"
    else:
        result = "crash"

    answer = {"result": result}

    for line in crashes:
        hit = CODE.search(line)

        if hit is not None:
            answer["code"] = hit.group(1)
            break

    for line in crashes:
        hit = AT.search(line)

        if hit is not None:
            answer["site"] = name(table, int(hit.group(1), 16))
            break

    for line in crashes:
        hit = FROM.search(line)

        if hit is not None:
            answer["under"] = "%s at %s" % (name(table, int(hit.group(1), 16)), hit.group(2))
            break

    return answer


def sweep(states, frames, timeout, samples=1):
    """Run each state, and where more than one sample is asked for, let the crash win.

    A state can pass on a run it usually fails. When the samples disagree, the crash is recorded
    and the state is marked flaky, with the split in its note.
    """
    table = symbols()
    found = {}

    for state in states:
        seen = []

        for _ in range(samples):
            code, lines = bringup_sweep.runOne(state, frames, timeout)
            seen.append(observe(table, code, lines))

        crashed = [one for one in seen if one["result"] != "clean"]

        if not crashed:
            found[state] = seen[0]
            continue

        answer = dict(crashed[0])

        if len(crashed) != len(seen):
            answer["flaky"] = True
            answer["note"] = ("%d of %d samples ran clean, so this state flaps on one build"
                              % (len(seen) - len(crashed), len(seen)))

        found[state] = answer

    return found


def describe(entry):
    if entry["result"] == "clean":
        return "clean"

    if entry["result"] == "aborted":
        return "aborted at the game's own fatal: %s" % entry.get("site", "reason not traced")

    return "%s %s in %s, under %s" % (entry["result"], entry.get("code", "?"),
                                      entry.get("site", "unresolved"),
                                      entry.get("under", "unresolved"))


def load():
    with open(BASELINE, encoding="utf-8") as handle:
        stored = json.load(handle)

    return {int(key): value for key, value in stored["states"].items()}


def save(found, previous):
    """Rewrite the baseline, carrying the flaky flags and notes the old one carried.

    Those two fields are the only hand-written data in the file.
    """
    states = {}

    for state in sorted(found):
        entry = dict(found[state])
        was = previous.get(state, {})

        for field in ("flaky", "note"):
            if field in was and field not in entry:
                entry[field] = was[field]

        states[str(state)] = entry

    body = {
        "comment": "Written by tools/bringup_check.py --save. Sites are a function and an "
                   "offset, never an address: the bring-up image is linked /OPT:REF and every "
                   "function moves when a translation unit is added. Set \"flaky\": true on a "
                   "state that has been seen to pass and fail on the same build, and say why "
                   "in its \"note\".",
        "states": states,
    }

    with open(BASELINE, "w", encoding="utf-8") as handle:
        handle.write(json.dumps(body, indent=4, sort_keys=False))
        handle.write("\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--states", default=None,
                        help="comma-separated states (default every one in the baseline)")
    parser.add_argument("--frames", type=int, default=60,
                        help="frames before the game posts its own quit (default 60)")
    parser.add_argument("--timeout", type=int, default=60,
                        help="seconds before a state is killed anyway (default 60)")
    parser.add_argument("--save", action="store_true",
                        help="rewrite the baseline from this sweep")
    parser.add_argument("--confirm", type=int, default=2,
                        help="re-runs of a newly failing state before it is called a regression "
                             "(default 2); every one of them must fail")
    parser.add_argument("--samples", type=int, default=None,
                        help="runs per state when saving (default 3); a state that does not "
                             "agree with itself is recorded as crashing and marked flaky")
    parser.add_argument("--stale-ok", action="store_true",
                        help="sweep even where a source is newer than the linked image")
    arguments = parser.parse_args()

    if not os.path.exists(LINKED):
        print("no build-bringup/out/HYPERX.exe.")
        print("Build it with tools/bringup_run.py. This checker needs a linked bring-up image")
        print("and the game's assets, so it cannot run on CI; it belongs in the preflight gate.")
        return 2

    if not os.path.exists(MAP):
        print("no build-bringup/out/HYPERX.map, so no crash address can be given a name.")
        print("Rebuild build-bringup; the map is what makes a baseline survive the next link.")
        return 2

    stale = bringup_sweep.staleSources(LINKED)

    if stale and not arguments.stale_ok:
        print("build-bringup/out/HYPERX.exe is older than %d modified source(s):" % len(stale))

        for one in stale[:5]:
            print("    " + one)

        print("Rebuild first. Sweeping a stale image is self-consistent and about the wrong "
              "binary.")
        return 2

    previous = {}

    if os.path.exists(BASELINE):
        previous = load()
    elif not arguments.save:
        print("no %s; run with --save on a build you have judged good"
              % os.path.relpath(BASELINE, ROOT))
        return 2

    if arguments.states is not None:
        states = [int(one) for one in arguments.states.split(",")]
    elif previous:
        states = sorted(previous)
    else:
        states = list(range(bringup_sweep.STATE_COUNT))

    bringup_run.stage()

    samples = arguments.samples

    if samples is None:
        samples = 3 if arguments.save else 1

    found = sweep(states, arguments.frames, arguments.timeout, samples)

    if arguments.save:
        save(found, previous)
        print("baseline written for %d states" % len(found))
        return 0

    regressed = []
    warned = []
    fixed = []
    moved = []
    unknown = []

    for state in states:
        now = found[state]
        was = previous.get(state)

        if was is None:
            unknown.append(state)
            continue

        if was["result"] == "clean" and now["result"] != "clean":
            # A regression fails every re-run; one clean re-run means a burst.
            burst = False
            again = now

            for _ in range(arguments.confirm):
                again = sweep([state], arguments.frames, arguments.timeout)[state]

                if again["result"] == "clean":
                    burst = True
                    break

            if burst:
                warned.append((state, "failed once and then ran clean, so a burst"))
                continue

            found[state] = again

            if was.get("flaky"):
                warned.append((state, "baseline calls this flaky: " + describe(again)))
            else:
                regressed.append((state, describe(again)))
        elif was["result"] != "clean" and now["result"] == "clean":
            if was.get("flaky"):
                warned.append((state, "baseline calls this flaky and it ran clean this time"))
            else:
                fixed.append((state, was.get("site", was["result"])))
        elif was["result"] != "clean" and now.get("site") != was.get("site"):
            # A flaky state that walks a corrupted stack faults somewhere different every run.
            if not was.get("flaky"):
                moved.append((state, was.get("site", "unresolved"),
                              now.get("site", "unresolved")))

    for state, why in sorted(regressed):
        print("REGRESSED   state %-3d %s" % (state, why))

    for state, why in sorted(warned):
        print("WARN        state %-3d %s" % (state, why))

    for state, where in sorted(fixed):
        print("NOW CLEAN   state %-3d was %s" % (state, where))

    for state, old, new in sorted(moved):
        print("MOVED       state %-3d %s -> %s" % (state, old, new))

    for state in sorted(unknown):
        print("UNKNOWN     state %-3d %s, and the baseline has never seen it"
              % (state, describe(found[state])))

    nClean = len([one for one in found.values() if one["result"] == "clean"])

    print("%d of %d clean; %d regressed, %d warned, %d now clean, %d moved, %d unknown"
          % (nClean, len(found), len(regressed), len(warned), len(fixed), len(moved),
             len(unknown)))

    if fixed or moved:
        print("Run --save once the new state is the one you mean to keep.")

    return 1 if regressed or unknown else 0


if __name__ == "__main__":
    sys.exit(main())
