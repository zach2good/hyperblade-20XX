# Symbols

Facts about the original that the build and the checkers read. Each CSV except `functions.csv`
opens with a comment saying what it holds;
[docs/ANNOTATIONS.md](../docs/ANNOTATIONS.md#where-exceptions-are-recorded) gives the rules for
adding a row.

The ones you'll meet first:

- `functions.csv`: every function in the original, with its address, size, calling convention
  and signature. The address and size are measured. `tools/pull_functions.py` refreshes the
  names from your Ghidra database. A written function's name is the source's; the rest are
  Ghidra's guesses.
- `retired.csv`: functions no source spelling has matched yet, and what each attempt measured.
- `float_constants.csv`, `static_guards.csv` and `library_data.csv`: data reccmp can't find in
  the original on its own. `reccmp-project.yml` reads them.

The rest record exceptions a checker would otherwise report, each with its reason.
