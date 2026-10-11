# Check that each header compiles on its own, with nothing included before it.
#
# A header that compiles only after another include, such as src/common/compat.h for Windows
# types, breaks the next file that includes it first. Adding the missing include costs no bytes:
# every .cpp already includes it and the guard skips the second copy.
#
# Not in tools/checkers.py, since it runs the VC6 compiler once per header. Run it after adding a
# header, moving a declaration between headers, or any include sweep.
#
#   python tools/check_headers_standalone.py
#   python tools/check_headers_standalone.py --list
#
# Read only. Exit code 1 when a header fails or CL.EXE is missing.

import glob
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def vc6_root():
    """VC6_ROOT from the environment, else the value the measured build cached."""
    if os.environ.get("VC6_ROOT"):
        return os.environ["VC6_ROOT"]

    cache = os.path.join(ROOT, "build", "CMakeCache.txt")

    if os.path.exists(cache):
        for line in open(cache, encoding="utf-8", errors="replace"):
            if line.startswith("VC6_ROOT:"):
                return line.split("=", 1)[1].strip()

    raise SystemExit("VC6_ROOT is not set and build/CMakeCache.txt names none")


CL = os.path.join(vc6_root(), "VC98", "Bin", "CL.EXE")
SYSTEM_INCLUDE = os.path.join(vc6_root(), "VC98", "Include")

# The build's flags without the optimiser, since this only asks whether the header parses.
FLAGS = ("/nologo", "/c", "/MT", "/GX", "/GR-", "/Zp8", "/W3",
         "/DWIN32", "/D_WINDOWS", "/DNDEBUG")

# Third-party code, which is not this tree's to make self-contained.
SKIP = ("jpeg/",)


def headers():
    for path in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.h"), recursive=True)):
        shown = os.path.relpath(path, os.path.join(ROOT, "src")).replace(os.sep, "/")

        if not shown.startswith(SKIP):
            yield shown


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    if not os.path.exists(CL):
        print("%s is not there, so nothing was compiled" % CL)

        return 1

    work = tempfile.mkdtemp(prefix="hbstandalone")

    env = dict(os.environ)
    env["TMP"] = work
    env["TEMP"] = work
    env["PATH"] = os.path.dirname(CL) + os.pathsep + env.get("PATH", "")

    probe = os.path.join(work, "probe.cpp")
    obj = os.path.join(work, "probe.obj")

    good = 0
    bad = []

    try:
        for shown in headers():
            open(probe, "w", encoding="utf-8").write('#include "%s"\n' % shown)

            run = subprocess.run(
                [CL] + list(FLAGS) + ["-I" + os.path.join(ROOT, "src"),
                                      "-I" + os.path.join(ROOT, "third_party", "jpeg-6a"),
                                      "-I" + SYSTEM_INCLUDE, "/Fo" + obj, probe],
                capture_output=True, text=True, cwd=work, env=env)

            out = run.stdout + run.stderr

            if "error" not in out.lower():
                good += 1
                continue

            first = ""

            for line in out.split("\n"):
                if "error" in line.lower():
                    first = re.sub(r"^.*?error", "error", line.strip())
                    break

            bad.append((shown, first))
    finally:
        for name in ("probe.cpp", "probe.obj"):
            whole = os.path.join(work, name)

            if os.path.exists(whole):
                os.remove(whole)

        if os.path.isdir(work) and not os.listdir(work):
            os.rmdir(work)

    if show or bad:
        for shown, first in bad:
            print("%-44s %s" % (shown, first[:74]))

    print("%d header(s) compile on their own, %d do not" % (good, len(bad)))

    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
