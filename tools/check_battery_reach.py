# Counts how many of the tree's source files each checker in the battery actually opens.
#
#     python tools/check_battery_reach.py
#     python tools/check_battery_reach.py --list
#
# A checker that reads nothing reports "clean", so an empty fault list cannot show a blind one.
# This runs each battery entry in its own process with `open` and `io.open` wrapped, and
# compares the set of src files it opened against the expected shapes: every source, every
# header, or both, each with or without src/jpeg/ (third-party code a checker may skip). One
# further shape is allowed for unused_includes.py, which judges only our .cpp files but must read
# every header. A checker that reads no source at all is registered below with what it reads,
# and one that reads a single fixed file is registered with that file.
#
# It is not in tools/checkers.py because it takes as long as the battery. Run it after changing
# a checker's file selection or moving files. Exits 1 on a checker whose reach fits no shape.

import glob
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

sys.path.insert(0, os.path.join(ROOT, "tools"))

# A checker that reads no source at all, with what it reads instead.
ELSEWHERE = {
    "check_doc_paths.py": "reads docs/ and tools/, and checks the paths their prose names",
    "check_stray_sources.py": "asks git what is tracked rather than opening anything",
    "index.py --check": "reads the index it maintains, not the files it indexes",
    "check_ledger_figures.py": "reads build/report.json and the two ledgers in symbols/",
    "check_lint.py": "runs reccmp's linter in a child process, which reads src/ itself",
}

# A checker that reads one fixed source file by design, with the file.
FIXED = {
    "plan_names.py": "the plan handler tables in src/planvm/planvm.cpp",
}

# A directory a checker skips on purpose, and why.
THIRD_PARTY = "jpeg"


def counted(pattern):
    return set(os.path.relpath(p, ROOT).replace(os.sep, "/")
               for p in glob.glob(os.path.join(ROOT, "src", "**", pattern), recursive=True))


def reach(entry, spyOut):
    """The number of src files one battery entry opens, run as its own process."""
    parts = entry.split()

    spy = (
        "import builtins, io, os, atexit\n"
        "seen = set()\n"
        "realOpen = builtins.open\n"
        "realIo = io.open\n"
        "root = %r\n"
        "def note(path):\n"
        "    try:\n"
        "        p = os.path.relpath(str(path), root).replace(os.sep, '/')\n"
        "    except Exception:\n"
        "        return\n"
        "    if p.startswith('src/') and (p.endswith('.cpp') or p.endswith('.h')):\n"
        "        seen.add(p)\n"
        "def spyOpen(path, *a, **k):\n"
        "    note(path)\n"
        "    return realOpen(path, *a, **k)\n"
        "def spyIo(path, *a, **k):\n"
        "    note(path)\n"
        "    return realIo(path, *a, **k)\n"
        "builtins.open = spyOpen\n"
        "io.open = spyIo\n"
        "atexit.register(lambda: realIo(%r, 'a', encoding='utf-8').write('%%d\\n' %% len(seen)))\n"
        "import runpy, sys\n"
        "sys.argv = %r\n"
        "runpy.run_path(%r, run_name='__main__')\n"
        % (ROOT, spyOut, parts, os.path.join(ROOT, "tools", parts[0]))
    )

    before = os.path.getsize(spyOut) if os.path.exists(spyOut) else 0

    subprocess.run([sys.executable, "-c", spy], capture_output=True, cwd=ROOT)

    if not os.path.exists(spyOut) or os.path.getsize(spyOut) == before:
        return None

    written = open(spyOut, encoding="utf-8").read().strip().split("\n")

    return int(written[-1])


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    import checkers as battery

    sources = counted("*.cpp")
    headers = counted("*.h")
    both = sources | headers

    def without(paths):
        return set(p for p in paths if not p.startswith("src/%s/" % THIRD_PARTY))

    allowed = {}

    for name, rows in (("the sources", sources), ("the headers", headers),
                       ("every source file", both)):
        allowed[len(rows)] = name
        allowed[len(without(rows))] = "%s but src/%s" % (name, THIRD_PARTY)

    # unused_includes.py judges only our .cpp files but reads every header, jpeg/ included,
    # because our sources include some of them.
    allowed[len(without(sources)) + len(headers)] = "the headers, and the sources but src/%s" \
                                                    % THIRD_PARTY

    handle, spyOut = tempfile.mkstemp(suffix=".txt")
    os.close(handle)
    os.remove(spyOut)

    faults = []
    rows = []

    try:
        for entry in battery.CHECKERS:
            n = reach(entry, spyOut)

            if n is None:
                faults.append((entry, "did not run to completion"))
                continue

            if n == 0:
                if entry in ELSEWHERE:
                    rows.append((entry, n, ELSEWHERE[entry]))
                    continue

                faults.append((entry, "opened no source file and has no reason registered"))
                continue

            if n == 1 and entry in FIXED:
                rows.append((entry, n, FIXED[entry]))
                continue

            if n not in allowed:
                faults.append((entry, "opened %d file(s), which is no set this tree has" % n))
                continue

            rows.append((entry, n, allowed[n]))
    finally:
        if os.path.exists(spyOut):
            os.remove(spyOut)

    if show:
        for entry, n, why in sorted(rows, key=lambda r: (-r[1], r[0])):
            print("%-40s %4d  %s" % (entry, n, why))

    for entry, why in faults:
        print("FAULT %-34s %s" % (entry, why))

    print("%d checker(s) in the battery, %d reading a set this tree has, %d fault(s)"
          % (len(battery.CHECKERS), len(rows), len(faults)))

    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
