# Contributing

We rebuild the C++ source that Visual C++ 6.0 SP3 compiled into `HYPERX.EXE`, then clean it into
typed, readable code. reccmp scores every function against the original, so every change is
measured. A change that makes the source clearer stays even when it costs bytes, and its commit
says how many.

This file holds every rule the project has. [AGENTS.md](AGENTS.md) is a short subset for AI
tools. [docs/README.md](docs/README.md) maps the other documents, and its
[glossary](docs/README.md#glossary) defines the terms used here: row, spelling, wall, bring-up
and the rest.

## Before you start

You need:

- the toolchain, your own copy of the game and reccmp, as [README.md](README.md#requirements)
  lists and [Setup](README.md#setup) installs;
- enough x86 assembly to read a diff, and some idea of how a 1998 compiler lays out C++;
- your own Ghidra project, for reading functions nobody has written yet
  ([docs/GHIDRA.md](docs/GHIDRA.md)).

The work is slow. One function can take a day of probing, and a change to a header moves rows in
files you never opened. [docs/MATCHING.md](docs/MATCHING.md) holds the technique and
[docs/ANNOTATIONS.md](docs/ANNOTATIONS.md) the marker rules.

The goal is accuracy. A pull request that fixes a game bug, modernises the code or adds a feature
will be closed. The bring-up build is the one place for fixes.

Keep pull requests small: one function, or one class, at a time. A reviewer has to check every row
the change moved.

## Rules for source

- **Format with clang-format 22** and the repository's `.clang-format`. CI checks it.
- **Write C++98 under `src/`.** No `auto`, lambdas, `constexpr`, `nullptr` or range-for. VC6 reads
  `auto` as the C storage class.
- **Write typed, readable code.** Name every field and parameter for what it holds, and use real
  types, not raw buffers, integer puns or offsets from `this`. Prefer the spelling that also keeps
  the bytes. Where none does, keep the typed spelling and record what it cost.
  [docs/RE_CRUFT.md](docs/RE_CRUFT.md) lists the shapes to remove.
- **Keep the original's statement order.** Don't move repeated code into helpers, and don't cache a
  value the original reads again.
- **Keep the program's behaviour.** Don't fix the original's bugs.
- **Mark each function.** Put `// FUNCTION: HYPERX 0xADDRESS` directly above each function you
  write. [docs/ANNOTATIONS.md](docs/ANNOTATIONS.md) lists the other marker kinds.
- **Every function is a member or lives in a named namespace.** Don't write `Name__Function` free
  functions: a `Name__` prefix means a member of `Name` or a function in `namespace Name`.
  Bring-up code lives in `namespace BringUp`.
- **One class, one `.h` and `.cpp` pair.** A namespace's functions live in one `.cpp` named for it.
  An exception goes in `symbols/layout_exceptions.csv` with its reason.
- **Link library code; never copy it.** The C runtime is LIBCMT and the JPEG code is IJG libjpeg
  6a. Pair their routines with `// LIBRARY:` markers against the linked library. Don't rewrite a
  runtime or library routine, static or public, to score it.
- **Write assembly only where the original was assembly.** An `__asm` block or a
  `__declspec(naked)` routine is allowed only where the original was hand-written assembly,
  proved by idioms VC6 never emits and listed in `symbols/original_asm.csv` with that evidence.
  Annotate it as [docs/ANNOTATIONS.md](docs/ANNOTATIONS.md#hand-written-assembly) says. Never
  write assembly to steer a compiled function's bytes. `python tools/check_asm_bodies.py`
  enforces this.
- **Avoid `volatile`.** Use it only for a value really shared with another thread, the hardware
  or a re-entered callback, with the reason on the line or the line above. Never use it to force
  a load or a store the original's scheduler happened to emit. `python tools/check_volatile.py` enforces this.
- **Put bring-up fixes inside `#ifdef HYPERX_BRINGUP`.** They must not change the measured image.
- **Source comments describe the code.** They never record scores, percentages or match history.
  Default to no comment. When one is needed, state the unusual fact about the original in one
  line. `python tools/check_score_prose.py` enforces this.
- **Treat a Ghidra name as a claim.** [docs/re_corrections.md](docs/re_corrections.md) lists the
  known wrong ones.
- **Never run `build/out/HYPERX.exe` or the original.** Both change the display resolution. Run
  the game only through the bring-up build, with `python tools/bringup_run.py` or
  `python tools/check_match.py --build`.
- Code under `tools/` is ordinary Python.

## Workflow for one change

1. **Pick a row**, one function in `symbols/functions.csv`. These tools rank the work:

   ```sh
   python tools/near_misses.py          # unfinished functions, by what closing them is worth
   python tools/fault_classes.py --open # rows outside the known walls, by real differences
   python tools/ready_unwritten.py      # unwritten functions whose callees exist (needs Ghidra)
   ```

   [docs/FUTURE_WORK.md](docs/FUTURE_WORK.md) lists the open work. Then check what already claims
   the address:

   ```sh
   python tools/claims.py 0xADDRESS
   ```

2. **Save the baseline**, the per-row scores you'll compare against, before you edit anything:

   ```sh
   python tools/check_regressions.py --save
   ```

3. **Write the function.** Read it from Ghidra (see [docs/GHIDRA.md](docs/GHIDRA.md)). A new
   source file goes at the end of `HYPERX_SOURCES` in `CMakeLists.txt`. Link order is part of the
   match, so never sort that list. Run `clang-format -i` on every file you touched.

4. **Probe.** A build and rescore takes minutes. A single-file probe takes seconds:

   ```sh
   python tools/probe_asm.py src/graphics/collisioncage.cpp --func ComputeFaceNormals
   ```

   It compiles one file to a `/FAs` listing with the build's own flags, and `--func` prints one
   function. Use it to sweep spellings, then confirm the winner with a build. From Git Bash, MSYS
   turns a `/flag` into a path and `cl` then compiles at `/Od`, so the tool passes every flag in
   the `-` form and refuses an unoptimised listing. A failed compile can leave the previous
   listing in place, so check the compile before you read the listing. Two more traps:

   - A spelling that computes a different value can score higher than the right one. Check that
     the variant is still the same program.
   - Declare nothing the probe doesn't read. VC6 deletes an unreferenced local, so a variant that
     adds one is the base under a different name.

5. **Build.** Don't pipe the build output: a build behind a pipe returns the pipe's status, and a
   failed build leaves the old image for the scorer to read. If NMake stops with `U1054`, run it
   again; `python tools/cmake_build.py build` does that for you.

   ```sh
   cmake --build build
   ```

6. **Score.** Always pass `--json`. Without it, `asmcmp` leaves the old `build/report.json` in
   place, and every figure you read is from the previous build.

   ```sh
   python -m reccmp.tools.asmcmp --target HYPERX --json build/report.json --silent
   python -m reccmp.tools.asmcmp --target HYPERX --verbose 0xADDRESS --no-color
   ```

7. **Check for regressions.** A change to a header, a global or a marker moves rows in files you
   didn't touch. The summary hides moves under a point, so compare every row with the baseline.
   `check_regressions.py` does this. It fails on any row more than a point below the baseline,
   partial rows too, and stops on a stale build, so read its first line. If a row drops, try
   another typed spelling. If none keeps the bytes, state the cost in the commit and save with
   `--accept-drops`; `--save` refuses a drop without it.

   ```sh
   python tools/check_regressions.py
   ```

8. **Run the checkers.** Don't pipe a checker through `head` or `tail` when you need its exit code.

   ```sh
   python tools/checkers.py
   python tools/check_markers_resolve.py
   ```

   `check_markers_resolve.py` resolves a marker by the line the function was compiled at, so
   rebuild after any edit, even to a comment, before you believe an UNRESOLVED line. Run
   `python tools/check_battery_bites.py` after a change to a checker, or to the source its probes
   anchor on. Run `cmake --build build --target verify` before you finish. Its score doesn't
   write `build/report.json`, so rescore with `--json` before you read a figure again.

9. **Build the bring-up image** after a merge, sort, split or rename. It compiles code the
   measured build skips.

   ```sh
   cmake --preset bringup    # once
   cmake --build build-bringup
   ```

A checker that reports a fault makes a claim. Reproduce it before you act on it.

## A good change

- It's small: one function, or one clean-up of one shape, per commit.
- `python tools/check_regressions.py` reports no drop, or the commit states what the drop cost.
- `python tools/checkers.py` passes, and `clang-format` leaves the source unchanged.
- It contains nothing from the original: no `HYPERX.EXE`, Ghidra project, disassembly listing,
  or reccmp HTML or JSON report. Addresses, names, sizes and counts are facts about the program
  and are allowed.
- It keeps the program's behaviour and stays within C++98.

CI checks the second point for you. It scores your pull request's image against a private copy of
the original, sets a `score` status, and comments with what `check_regressions.py` found.

## Where records go

A spelling that failed goes in `symbols/retired.csv`, one row per function, with the columns
`address,name,wall,evidence`. The `evidence` column says what you tried and what each spelling
measured. Don't record failed spellings in source comments or in the docs. Remove the row when the
function reaches an exact match; `check_regressions.py` reports a retired row that is exact.

A file that keeps another class's body goes in `symbols/layout_exceptions.csv`, with the columns
`path,reason`. List a file only when moving the body would make a different translation unit from
the original's.

[docs/ANNOTATIONS.md](docs/ANNOTATIONS.md#where-exceptions-are-recorded) gives the full rules for
every register under `symbols/`.

## Commit messages

Commit one matched function at a time. The title takes one of three forms:

```txt
Match `SWRend::DrawLine`
Partially match `MeshBone::DrawFaces`(69.46)
Docs: MATCHING.md reads a copied counter as a for loop
```

- `Match` names the one function the commit makes exact. When the change closes a second function
  too, name the one you worked on and list the other in the body.
- `Partially match` names a function that moved but didn't close. The number is reccmp's
  percentage after the change, with two decimals and no space before the bracket.
- `Scope: what changed` covers everything else. The scope is the area of the code, such as
  `Source`, `Symbols`, `Tools`, `Docs` or `Build`, not a language or a file type. Separate several
  scopes with commas: `Source, Symbols: ...`.
- Add a body only when the title can't carry the reason, and keep it to one or two lines.
- Use ASCII only, with British spelling. No em dashes, en dashes or arrows.
- Don't add `Co-Authored-By` or any trailer that credits an AI tool.
- In a shell, quote the title in single quotes. Bash runs text between backticks inside double
  quotes, and the commit still succeeds with the name gone.

## Licence

The project is licensed under LGPL-3.0-only. See [LICENSE](LICENSE) and [NOTICE](NOTICE).
