# hyperblade-20XX

<p align="center">
  <img src="docs/hb_20XX_logo.png" alt="HyperBlade 20XX" width="320">
</p>

<p align="center">
  <a href="https://github.com/zach2good/hyperblade-20XX/actions/workflows/ci.yml"><img src="https://github.com/zach2good/hyperblade-20XX/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
</p>

HyperBlade (1996), rebuilt and improved for modern systems. Built on top of
[hyperblade-decomp](https://github.com/zach2good/hyperblade-decomp).

You need your own copy of the game. Not affiliated with Activision or WizBang! Software Productions.

<p align="center">
  <img src="docs/gameplay_1080p.png" alt="A match on the hardware renderer" width="640">
</p>

## Features

- Drops into your existing HyperBlade install
- Supports all releases (original disc release (v1.0.0.0), Windows 2000 patch (v1.0.0.1), both with or without the Voodoo2 launcher (HB3DFxV2.exe))
- 64-bit Windows and Linux builds
- In-game debug menu on F2
- Every cheat code in the debug menu
- Runtime toggles for:
  - Original software renderer
  - New hardware renderer
  - Textured models
  - Gouraud shading/Phong shading
  - Rok emits light
  - Realtime shadows
- Widescreen support
- HUD and menu scaling
- Xbox, PlayStation and Switch controller support
- Music tracks can play from local rips, rather than the CD
- Bot spectator mode, where every dromer plays on the AI plans. Turn it on in the debug menu, start
  with `-b`, or press Tab for the side view
- Fixes for the bugs in the shipped AI plans, so AI carriers pass and shoot. The debug menu switches
  between them and the plans as shipped ([docs/plan_bugs.md](docs/plan_bugs.md))
- G, or a pad's left shoulder, sends your goalie up-ice after the rok and calls it back, with the
  fixed plans on
- Career stats: every Gauntlet match adds to each player's totals, saved in
  `hyperblade_20XX_career.txt`. On the stats panel, the arrows step through home, away, home
  career and away career
- Skippable intro splash screens
- Uncapped frame rate, with the game still simulated at 30 Hz
- Starts on the hardware renderer with realtime shadows, the uncapped frame rate and the plan
  fixes, where a GPU allows. The debug menu turns each off, and the choice is saved
- Various bug and crash fixes

## TODO

- Update menus to support controller button re-binding
- Play-test the Linux build, which builds and passes its tests but has not been run on a desktop
- Mac and browser builds via Emscripten
- Net and local multiplayer
- AI balance fixes
- The original RC, debug and plan menus in the debug menu

## Install

Download `hyperblade_20XX.zip` for Windows, or `hyperblade_20XX-linux.tar.gz` for Linux, from
[Releases](https://github.com/zach2good/hyperblade-20XX/releases) and unpack it into your HyperBlade
folder, beside `HYPERX.EXE`. Then run `hyperblade_20XX`. Keep `20XX_data.pak` beside it: it holds
our fonts and controller icons.

The first run reads the game's text out of `HYPERX.EXE` into `hyperblade_20XX_strings.txt`, and
later runs load that file.

To play the music without the CD, rip the audio tracks to `Data/Music/Track02.flac`,
`Track03.flac` etc. FLAC, MP3 and WAV are all supported. On Linux the rips are the only music, as
the CD drive is read on Windows alone.

## Simulating matches

`-sim` plays AI-only matches with no window or sound, about 2,000 times faster than real time on one core, and
writes every player's match stats to `sim_results.csv` beside the executable:

```sh
hyperblade_20XX -sim LOSAN SEATTLE 100 1 fixed
```

The arguments are the home team, the away team, the number of matches, the first seed (each
match adds one), `fixed` or `shipped` plans, optionally `arena=TEAM` to play in that team's drome rather than the
home team's, and optionally a file ending in `.csv` to write in place of `sim_results.csv`. A
team can play itself. A team is its file name, its name or its index;
an unknown team lists them all. The match settings in `booie.ini`, such as difficulty and period
length, still apply, and a simulation never writes the file. Each row is one player in one match, with the difficulty and period length it
was played at, their health at the end (100 is unhurt), the score, and the game's own match stats. A seed does not yet replay a match exactly.
Errors print instead of opening a dialog. A fatal one ends the run with exit code 3 and keeps the matches
already written.

## Build

The build needs nothing from the game. Each preset in [CMakePresets.json](CMakePresets.json)
configures, builds and tests, and the `package` target writes the drop-in archive to
`packages/` in its build folder.

### Windows

You need Visual Studio 2022 or later with the C++ x64 tools, CMake 3.25 or later, and Ninja. From
an x64 Developer Command Prompt:

```sh
cmake --preset msvc-x64
cmake --build --preset msvc-x64
ctest --preset msvc-x64
cmake --build --preset msvc-x64 --target package
```

### Linux

[docker/linux.Dockerfile](docker/linux.Dockerfile) holds GCC 14, Clang 20 and SDL's build
dependencies. It builds, tests and packages with the preset `PRESET` names, `linux-gcc` by default:

```sh
docker build -t hyperblade-linux -f docker/linux.Dockerfile docker
docker run --rm -e PRESET=linux-gcc -v "$PWD:/src:ro" -v "$PWD/.cpm-cache-linux:/cpm" -v "$PWD/build-linux:/build" hyperblade-linux
```

On a Linux machine with those tools, the `linux-gcc` and `linux-clang` presets work as the Windows
one does. GCC and Clang build with `-Wall -Werror`, and MSVC with `/W4 /WX`.

### Tests

The `unit` checks need nothing. `gpu` draws through SDL's GPU API and reports itself skipped with
no device. The `data` checks read the game's own files and run only when `HYPERX_GAME_DATA` names
your HyperBlade folder, which also links its folders in beside the built image:

```sh
cmake --preset msvc-x64 -DHYPERX_GAME_DATA=C:/Games/HyperBlade
ctest --preset msvc-x64 -L data
```

## Tools

These need Python 3.12 or later. Some read your `HYPERX.EXE`, so copy `hyperx-user.example.yml` to
`hyperx-user.yml` and point it there.

```sh
python tools/extract_rc.py          # menu, dialogs, strings, version and icon, as an .rc script
python tools/extract_mdm1.py --png  # every MDM1-packed bitmap in Data, as BMP and PNG
python tools/plan_dump.py           # the AI plans, printed as pseudo-source
python tools/make_icon.py           # redraws the Rok icon into res/
```

[docs/](docs/README.md) holds what we know about the game: the class hierarchy, the other files on
the disc, and the bugs in the shipped AI plans.

## Legal

This project is for preservation, research and interoperability. It holds no original binary,
game data or disassembly. Don't attach any of these to an issue or a pull request, and don't ask
for a copy of the game.

LGPL-3.0-only for this project's own work. [NOTICE](NOTICE) says what the licence covers.
HyperBlade belongs to its rights holders.

`20XX_data.pak` carries two CC0 sets: the [Pixel Operator](res/fonts/PixelOperator-LICENSE.txt)
fonts and Kenney's [Input Prompts](res/prompts/License.txt).
