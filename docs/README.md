# Documents

Start with the first five, in order: they cover the work itself. The rest are reference, read
when a task needs them. The plan bug reports are findings about the game, not about the
decompilation.

| Order | Document | Read it for |
| --- | --- | --- |
| 1 | [CONTRIBUTING.md](../CONTRIBUTING.md) | Every rule, the workflow for one change, where records go, commit rules |
| 2 | [GHIDRA.md](GHIDRA.md) | Setting up your own Ghidra project, reading it, and publishing names to it |
| 3 | [MATCHING.md](MATCHING.md) | How scoring works, the levers grouped by the symptom in a diff, and the walls |
| 4 | [ANNOTATIONS.md](ANNOTATIONS.md) | The marker rules, and the registers under `symbols/` |
| 5 | [RE_CRUFT.md](RE_CRUFT.md) | Decompiler shapes to clean out of the source, and what is left of each |
| 6 | [FUTURE_WORK.md](FUTURE_WORK.md) | Open work |
| 7 | [re_corrections.md](re_corrections.md) | Ghidra readings that are wrong, with the evidence |
| 8 | [class_hierarchy.md](class_hierarchy.md) | Vtable layouts and the class hierarchy |
| 9 | [v1000_debug_directory.md](v1000_debug_directory.md) | The FPO data in v1.0.0.0 and what it tells us about v1.0.0.1 |
| 10 | [OTHER_MODULES.md](OTHER_MODULES.md) | The other executables and DLLs on the disc, and why they're out of scope |
| 11 | [plan_bugs.md](plan_bugs.md) | Bugs in the shipped AI plans, with the plan VM rules they rest on |
| 12 | [plan_bugs_skaters.md](plan_bugs_skaters.md) | Findings in the skater and goalie plans |
| 13 | [plan_bugs_objects.md](plan_bugs_objects.md) | Findings in the object plans and the team ratings files |

[tools/README.md](../tools/README.md) describes every tool, [src/README.md](../src/README.md)
what each source folder holds, and [symbols/README.md](../symbols/README.md) the CSVs the build
and the checkers read.

## Glossary

| Term | Meaning |
| --- | --- |
| the original, the client | The retail `HYPERX.EXE` v1.0.0.1. "The client" is the original's side of a diff. |
| this build, ours | The image this repository builds, `build/out/HYPERX_sym.exe` for scoring. |
| row | One function in `symbols/functions.csv`, and its line in reccmp's report. |
| marker | A comment such as `// FUNCTION: HYPERX 0x00401000` that pairs source with an address. See [ANNOTATIONS.md](ANNOTATIONS.md). |
| exact | A row whose bytes match the original's, 100 per cent. Only exact rows count in the byte share. |
| effective | A row reccmp scores 100.00 that still differs in something it judges harmless, such as a register name. It is not exact. |
| near miss | A row close to exact. `tools/near_misses.py` ranks them by what closing them is worth. |
| spelling | One way to write a piece of source. Two spellings of one program can compile to different bytes. |
| lever | A fact about VC6 that a spelling can use to reach the original's bytes. [MATCHING.md](MATCHING.md) lists them. |
| wall | A residue no known spelling reaches. See [Known walls](MATCHING.md#known-walls). |
| residue | The instructions that still differ between the two sides of a row. |
| retired | A row whose last gap was measured and given up, recorded in `symbols/retired.csv` with its evidence. |
| probe | A single-file compile to a listing with `tools/probe_asm.py`, to compare spellings in seconds. |
| baseline | The saved per-row scores that `tools/check_regressions.py` compares a build against. |
| battery | The structural checkers `tools/checkers.py` runs. |
| fold | The linker giving several identical functions one address. The original's link folds some bodies; see [Known walls](MATCHING.md#known-walls). |
| cruft | Source that compiles to the right bytes but reads like a memory map. See [RE_CRUFT.md](RE_CRUFT.md). |
| register | A CSV under `symbols/` that records a measured exception, such as `retired.csv`. See [ANNOTATIONS.md](ANNOTATIONS.md#where-exceptions-are-recorded). |
| FPO | Frame pointer omission. v1.0.0.0's debug directory holds an FPO record for each function without a frame pointer. See [v1000_debug_directory.md](v1000_debug_directory.md). |
| CRT, LIBCMT | The Microsoft C runtime, linked from VC6's `LIBCMT.LIB` and never rewritten here. |
| bring-up | The second build, with `HYPERX_BRINGUP` defined, that runs the game on a current machine. |
| HyperBlade | The game: a team sport on skates, two teams of two dromers and a goalie each. |
| dromer | A skater. Each team has two dromers on the floor, slots 0 to 3. |
| Rok | The ball. The skater holding it is the Rok carrier. |
| drome | An arena, the scene object `id_MOD_Drome`. |
| ADLIB | Authoring and Design Language for Interactive Behavior: the developers' name for the plan language, its compiler and its run-time system. See [Sources](#sources). |
| plan | An ADLIB Plan: an AI script in `Data/PLANS`, compiled to p-code in a `.PBN` file. Skaters, objects and cameras each run one. |
| plan VM | ADLIB's run-time system in `src/planvm/`, which runs the plans: behaviours, interactors and agenda items. The p-code compiler, `PlanVM`, ships inside the game. |
| behaviour | One state of a plan, made of agenda items that run together. An object runs one behaviour at a time. |
| interactor | A handler inside a behaviour that reacts to a message or a collision. |
| agenda item | One task a behaviour runs, such as `CGlideOnSurface` or a timer: the article's Activity. ENUMIDS.H lists them as `AgendaItemIDs`, ending with `id_AGD_CTimerActivity`. |
| PBN | The compiled plan file format: ADLIB's p-code. |
| pronoun | A plan word tagged `0xfedc` that names an object relative to the actor, such as `id_PRN_MyGoal`. A resolver turns it into a slot at run time. |
| interactor name | A name `id_SET_InteractorName` gives an interactor, so a later word tagged `0xdcba` can refer to it. |
| MDM1 | The LZSS packing on most of the game's bitmaps. `tools/extract_mdm1.py` unpacks it. |
| ENUMIDS.H | The developers' header, shipped in the game's `DATA` directory, that names the plan VM's ids: messages (`id_MSG_`), pronouns (`id_PRN_`), ratings (`id_RAT_`) and the rest. |
| slot | An index into a table: an object slot, a vtable slot, or a frame slot on the stack. |

## Sources

The developers described ADLIB in Stuart Rosen and Robert Duisberg, "Bringing Life to HyperBlade",
*Game Developer*, August/September 1996, pp. 22 to 25. The issue is on GDC Vault:
<https://media.gdcvault.com/GD_Mag_Archives/GDM_AugSept_1996.pdf>. It names the system's parts
(plans, behaviours, interactors, activities, decision functions, the built-in p-code compiler) and
how a plan becomes a network of linked objects when an object adopts it. Its one code listing is
in a syntax the shipped compiler rejects.

The ADLIB reconstruction contributors, *libadlib: An Independent Reconstruction of the ADLIB
Behavior Runtime*, version 0.1.0, 2026-10-03, MIT licence,
<https://github.com/vomitselfie/libadlib>. It reconstructs ADLIB from a different build of
`HYPERX.EXE` (1,359,872 bytes, 1996-10-21), so its addresses do not apply here, and most of its
function names are its own. We take a name from it only where the binary's strings, ENUMIDS.H or
the article back it, and check the behaviour in our own source.
