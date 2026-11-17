# The August 1996 demo

A third build of `HYPERX.EXE` survives: the playable demo from PC Gamer Disc 2.10, linked
1996-08-21, two months before v1.0.0.0. Nothing in `src/` targets it yet. This page records what
sets it apart.

## The image

| | Demo | v1.0.0.0 |
| --- | --- | --- |
| Linked | 1996-08-21 | 1996-10-21 |
| Linker | 3.00, incremental | 3.00, incremental |
| Jump thunks at the start of `.text` | 2,524 | 3,042 |
| `.text` size | 0xce24e | 0xf87fe |
| Debug directory | None | FPO, CodeView, misc |
| Version resource | 5.0.3.1, "NET ALPHA", private build "BETA", WizBang! | 1.0, Activision |

Both came from the same compiler. A function the two builds share has the same bytes once
relocations and call targets are masked, down to the `int3` padding. The demo has no FPO table,
so its function starts come from the thunk targets and direct call targets instead.

## Pairing with v1.0.0.0

A first pass pairs 876 of the demo's 3,219 function starts with v1.0.0.0 functions, and 625 of
those land on a function the tree names. It takes, in order:

- Functions whose masked bodies are identical and unique on each side (366).
- Shared string literals, as `tools/fpo_anchors.py` does.
- Identical instruction sequences, ignoring operands.
- Votes along the call graph, as `tools/fpo_propagate.py` does.

729 of the 876 pairs agree on size within 25%, which the matcher never reads. Most unpaired
functions are code whose struct offsets moved, so a class with no pairs is not proof that the demo
lacks it.

## What the demo has

- **An instant replay screen.** Buttons `camra1` to `camra4`, `rewind`, `forwrd`, `play`,
  `pause`, `stop`, and clip buttons `savclp`, `lodclp` and `gamclp`. Their loaders are cases of a
  20-way screen switch that the demo's `Game::UpdateState` calls. v1.0.0.0 keeps only the unused
  strings "INSTANT REPLAY" and "Replay Screen".
- **A controls screen.** Joystick and keyboard frames `cntjs01` to `cntjs15` and `cntkb01` to
  `cntkb15`, on `cntbkg`.
- **Eleven 15-frame image sequences**: `confu`, `elect`, `fstln`, `lpass`, `lshot`, `slwln`,
  `slwrk`, `stick`, `stun`, `x2rok` and `x4rok`, loaded through a function table in `.rdata`.
- **Two teams.** Logo and HUD art for `sea` and `los` only, a home and an away team file in place
  of `TEAMS.CAT`, and intro screens `DemoInt1` to `DemoInt6`.
- **An input action table** of `amg_` names (`amg_HIP_CHECK`, `amg_LEG_SWEEP`, `amg_PASS` and the
  rest) beside `VK_` key names.
- **Debug keys** in the main window procedure: "Debug Graphics Turned On", "Real time illumination
  of drome in effect", and a toggle between the default and an alternate key configuration.
  Pause and game over are text messages ("Hit 'q' to continue").
- **An older network setup.** Dialog 602 is "Multiplayer Setup (temporary placeholder; will have
  new holo-style art)", and the `NET_CONNECTION` and `NEW_HOST` dialogs are still in. The demo
  doesn't compare `PLANS.CAT`, `ENUMIDS.H`, `COMMON.CFG` or the team and arena files between host
  and client.

## What only v1.0.0.0 has

Save and load (`SAVE%02d.BIN`), game options and volume settings, the cheats ("Hidden teams
enabled"), the gauntlet (`gauntlet.cfg`), jumbotron adverts (`jumbo.txt`), the league,
championship, tournament and head-to-head setup screens, the full team list, the camera track
classes (`CTrack`, `CWatch`, `CCircleTrack`) and the CD checks. String table entries 97 to 102 and
159 to 173 are not in the demo, and its menu has no "Display HUD Art".

## Next

- Add a `HYPERXDEMO` target to `reccmp-project.yml` once a function carries a demo marker.
- Raise the pair count. A matcher that reads struct offsets as fuzzy operands should pair most of
  the shifted code.
- Decode the replay screen. Neither release has anything like it.
