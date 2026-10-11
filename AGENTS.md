# Notes for AI tools

This repository rebuilds the source that Visual C++ 6.0 SP3 compiled into `HYPERX.EXE`.
[CONTRIBUTING.md](CONTRIBUTING.md) holds every rule and wins any disagreement. These are the rules
a tool most often breaks.

- Measure every change. Build with `cmake --build build`, rescore with `--json`, and run
  `python tools/check_regressions.py`. Compare every row, not the summary. Don't pipe the build or
  a checker when you need its exit code.
- Run `python tools/checkers.py` and `python tools/check_markers_resolve.py` after a build,
  `python tools/check_battery_bites.py` after a change to a checker, and
  `cmake --build build-bringup` after a merge, sort, split or rename.
- Run `clang-format -i` (version 22) on every source file you touch. Keep each marker on its own
  line above what it marks.
- Write typed, readable C++98 in `src/`, in the original's statement order. Where no typed
  spelling keeps the bytes, keep the typed one and state the cost in the commit.
- Never run `build/out/HYPERX.exe` or the original. Use the bring-up build.
- Treat a Ghidra name or a checker fault as a claim. Check it before you act on it.
- Record a failed spelling in `symbols/retired.csv`, never in a comment or a doc.
- Link library code; never copy it. Pair runtime and JPEG routines with `// LIBRARY:` markers.
- Write assembly only where the original was assembly, as listed in `symbols/original_asm.csv`.
- Avoid `volatile`, except for a value really shared with another thread or the hardware.
- Every function is a member or lives in a named namespace: no `Name__Function` free functions.
  One class, one `.h` and `.cpp` pair.
- Commit nothing from the original. Commit one matched function at a time, titled as
  [CONTRIBUTING.md](CONTRIBUTING.md#commit-messages) says. Never add `Co-Authored-By` or any
  trailer that credits an AI tool.
