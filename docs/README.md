# Documents

Findings about the game and its code, carried over from the decomp.

| Document | Read it for |
| --- | --- |
| [re_corrections.md](re_corrections.md) | Ghidra readings that are wrong, with the evidence |
| [class_hierarchy.md](class_hierarchy.md) | Vtable layouts and the class hierarchy |
| [OTHER_MODULES.md](OTHER_MODULES.md) | The other executables and DLLs on the disc |
| [plan_bugs.md](plan_bugs.md) | Bugs in the shipped AI plans, with the plan VM rules they rest on |
| [plan_bugs_skaters.md](plan_bugs_skaters.md) | Findings in the skater and goalie plans |
| [plan_bugs_objects.md](plan_bugs_objects.md) | Findings in the object plans and the team ratings files |

[src/README.md](../src/README.md) says what each source folder holds.

## Glossary

| Term | Meaning |
| --- | --- |
| the original, the client | The retail `HYPERX.EXE` v1.0.0.1. "The client" is the original's side of a diff. |
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
