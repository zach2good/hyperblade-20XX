# What v1.0.0.0 leaves on the cutting room floor

v1.0.0.0 links every function it compiled, called or not, so it keeps code the shipped game never
runs. The resources in both releases also keep dialogs, menu items and strings that nothing uses.
This page lists what is worth restoring or finishing in a port. [v1000.md](v1000.md) explains the
v1.0.0.0 build and its markers.

Every v1.0.0.0 game function has been looked at. What is not listed here is compiler output
(static initialisers, deleting destructors, unwind funclets), the C runtime, or a function both
releases share.

## Unfinished features in code

The code is written under `#ifdef HYPERX_V100`. Nothing in either release calls it.

| Feature | Where | State |
|---|---|---|
| Season, tournament and secret games | `GameCatalog` (`src/engine/gamecatalog.cpp`) reads `year.cat`: `SeasonCount`, `TournamentCount`, `SecretCount`, then `DefineGame` blocks of `HomeTeamID`, `AwayTeamID`, `gameindex`, `HomeScore`, `AwayScore` | Unfinished. It sizes the table for all three counts, then writes every game into one record and drops `gameindex`. No `year.cat` ships. The unused strings "Season Game", "Season Standings" and "Tournament Standings" go with it |
| Debug overlay | `DebugOverlay` (`src/app/debugoverlay.cpp`) | Per-dromer energy (`Dr0 E: %2.1f`), polygon counters (total, drawn, in view, back-facing, clipped), two scrolling message logs and a stack of text lines |
| Animation trace | `AnimPlayer::PrintAnimDebug` | Prints `Que anim: %s` and `Set anim: %s` for one named object |
| Palette and asset dumps | `ColorPalette::WriteRangeFiles`, `WritePaletteFile`, `WriteCustomPalette` | Write `Arenas.wbp`, `Objects.wbp`, `TeamA.wbp`, `TeamB.wbp`, `theRok.wbp`, `Open.wbp`, `General.wbp` and `Custom.pal` as text palettes |
| Model material dump | `ModelComponent::PrintMaterials`, `ReadFirstOrFail`, `MatchKeyword` | Prints a model's materials and texture names |
| Key file writer | `KeyConfigDialog::SaveKeys`, `SaveKeysAs` | Writes the key file `Config::ParseFile` reads. The overwrite test is the wrong way round |
| WAV writing | `Wave::Create`, `Write`, `CloseWrite`, `Save` | Writes `.wav` files |
| Meter indicator | `PassBar::LoadFromFile` and its overrides | A HUD meter surface ("Cannot create surface for Meter indicator!") that v1.0.0.1 no longer creates |
| Animated bitmaps | `FrameBitmap` (`src/graphics/framebitmap.cpp`) | A frame-sequence bitmap class (`%s%02d` frames) |
| Depth sorting | `DepthSort` (`src/graphics/depthsort.cpp`) | Bucket sort of faces before `MeshBone::DrawFaces`, from an older renderer |
| GIF loading | `CGPalette::ReadGifImage` and the other block readers | Reads GIF image, control, plain-text, application and comment blocks |
| Network guard | `GameEngine::ForceNetBehaviorChange` | "Too many forced net Behavior changes in this frame!" |
| Page manager | `PageMngm::Swap`, `ClearBuffer`, `Setup` | Stubs that only report they are stubs |

## Dialogs nothing opens

The dialogs are in both releases' resources. Neither release opens them.

| Dialog | Contents |
|---|---|
| 463, "Crash Test" | A collision tester: my and his initial speed, distance and approach direction (as a clock position), the last animation played, the hit level and the hit zone |
| 137 | An audio-visual options panel: music, sound, announcer, camera angle, resolution, detail, shading and texture, each a drop-down |
| 122 | A chooser with two buttons, Game Options and Audio Visual Options. Menu command 122 is an empty case in v1.0.0.0's `MainWindow::OnCommand` |

## Menu items

- "2 Player Mode" (command 155) is in the menu, and neither release handles it.
- "One on One" (104), "Two on Two" (105) and the network "Connection..." (138) and "Players..." (140)
  items have no handler in either release.
- v1.0.0.0 handles command 159 with an empty case. No menu item has that number.
- The resolution menu keeps a greyed 480x360 mode.

## Strings nothing loads

94 of the string table's 117 entries are never loaded by v1.0.0.1. Together they outline an
earlier front end:

- "Version 5.0.2.2 DEVELOPMENTAL".
- Menus: "Studio", "Trophy", "Create Team", "LOBBY", "PLAYER TUNNEL", "Locker Room", "Team Setup",
  "Holographic Viewer", "Replay Screen", "DynaBook".
- In-match: "INSTANT REPLAY", "SUBSTITUTIONS", "TIME OUT".
- Scouting: "SCOUTING REPORT" over ATTACK, DEFENSE, SHOOTING, PASSING, STEALING, CHECKING, SPEED
  and OVERALL.
- Modes: "Exhibition Game", "Season Game", "Tournament Game", "Season Standings", "Tournament
  Standings".
- An object picker list: "Dromer 0" to "Dromer 5", "Rok", "North Goal", "South Goal", "Camera",
  "God", "SoundMan", "Helmet Cam", "Multiplier", "Turnstile", "StartGun".
- Help for the animation assembly files: assemblies, triggers and segments, with their columns.
- Two more quit taunts: "Afraid of losing your head?" and "Can't hack it?".

## Behaviour that changed between releases

These live functions differ. They are marked in both releases but not yet fenced, so the
v1.0.0.0 build still runs v1.0.0.1's version.

- `Texture::SetColorMode`, `SetScrollColorMode` and `SetFillColor` turn colours into surface pixel
  values, part of the Windows 2000 text fix.
- `DDraw::FlipOrBlt` has no full-screen test, so a windowed game flips.
- `SoundChannel` is 0x4c bytes, without the ring fields.
- `UIScreen::~UIScreen` does not free `m_pLockedFlags`.
- `AwardsUIScreen::OnExitTabClick` tests a getter's address, not its result, so it always goes to
  the drome exterior.
- `SaveLoadUIScreen` keeps its return state at +0x470.
- `Spline::EvalUpperHalf` and `EvalLowerHalf` order their square root differently.
