# Find source files under src/ that git does not track, or that sit at the top of src/.
#
# Every tool globs `src/**/*.cpp`, so an untracked copy is read twice: sweeps double-count, a
# checker's zero covers a file nothing builds, and an edit can land in a copy CMakeLists never
# compiles. The build stays green, because CMakeLists names its sources one by one. A generator
# aimed at the old flat layout lands at the top of src/, where only the four files below belong.
#
# Gates at zero. A new source becomes the tree's when it is added to git and to CMakeLists.

import glob
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# The two that belong at the top of src/.
TOP = ("globals.cpp", "globals.h")


def untracked():
    """Every untracked source under src/, as git sees it."""
    answer = subprocess.run(
        ["git", "ls-files", "--others", "--exclude-standard", "--", "src"],
        cwd=ROOT, capture_output=True, text=True)

    return sorted(line.strip() for line in answer.stdout.splitlines()
                  if line.strip().endswith((".cpp", ".h")))


def main():
    os.chdir(ROOT)

    stray = untracked()
    top = sorted(os.path.basename(p) for p in glob.glob(os.path.join("src", "*.cpp"))
                 + glob.glob(os.path.join("src", "*.h"))
                 if os.path.basename(p) not in TOP)

    for path in stray:
        print("UNTRACKED %s: git does not have it, so every src/**/*.cpp scan reads it twice"
              % path)

    for name in top:
        print("AT THE TOP src/%s: the subsystem split left only %s there"
              % (name, ", ".join(TOP)))

    print("%d untracked source(s) under src/, %d file(s) at the top of src/ that do not belong"
          % (len(stray), len(top)))

    return 1 if stray or top else 0


if __name__ == "__main__":
    sys.exit(main())
