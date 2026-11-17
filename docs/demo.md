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

- **A game clips viewer**, with no working controls. See [below](#the-game-clips-viewer).
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

## The game clips viewer

The demo's screen factory at 0x0047f5e0 switches on the engine state at 0x004f32d0 and stores the
new screen at 0x004eb3dc, as `GameApp::InstantiateState` does. The lobby (3) and main menu (4)
keep their numbers, and the clips viewer moves from 12 to 10. These are the post-match screens:

| State | Constructor | Vtable | Screen |
| --- | --- | --- | --- |
| 10 | 0x004743a0 | 0x004d3a08 | Landing: the four tabs only |
| 11 | 0x00479f90 | 0x004d3d38 | Info tab: roster and arena pages |
| 12 | 0x004072f0 | 0x004d0028 | Game clips viewer |
| 13 | 0x0042dd70 | 0x004d1838 | Awards tab |

Each of the four screens draws the `Hviewl` holo frame and the tabs Info, Game Clips (`gamclp`),
Awards and Done. Vtable slots 9 to 12 are the tab handlers: Info goes to state 11, Game Clips to
12, Awards to 13 and Done to the lobby, state 3. Each records state 10 at 0x004eb3d8 as the state
to return to. Lobby widget 0 goes to state 10, so a player can reach the viewer.

The viewer adds `stop`, `rewind`, `play`, `pause` and `forwrd` buttons, clip buttons `lodclp` and
`savclp`, and camera buttons `camra1` to `camra4`. Every handler behind them is a bare `ret`, and
so is every slot it shares with the other post-match screens, 0x00407c80 to 0x00407da0. No
string in the demo names a clip file. The viewer is a front end with nothing behind it.

v1.0.0.1 keeps the state as `ESTATE_GAME_CLIPS_VIEWER`, 10, and `GameApp::InstantiateState`
stops the game with "Invalid state detected: GameClipsViewer". v1.0.0.0 has none of the viewer's
art names. Its string table keeps "INSTANT REPLAY" and "Replay Screen", which nothing loads.

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
- Map the rest of the demo's screen states onto v1.0.0.1's.
