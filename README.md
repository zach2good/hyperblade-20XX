# hyperblade-20XX

<p align="center">
  <img src="docs/hb_20XX_logo.png" alt="HyperBlade 20XX" width="320">
</p>

<p align="center">
  <a href="https://github.com/zach2good/hyperblade-20XX/actions/workflows/ci.yml"><img src="https://github.com/zach2good/hyperblade-20XX/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
</p>

HyperBlade, the 1996 game, built with a current compiler so we can fix and change it. This is a
fork of [hyperblade-decomp](https://github.com/zach2good/hyperblade-decomp), which rebuilds
`HYPERX.EXE` byte for byte with Visual C++ 6.0.

You need your own copy of the game. Not affiliated with Activision or WizBang! Software
Productions.

<p align="center">
  <img src="docs/gameplay_soft.png" alt="A match in the software renderer" width="640">
</p>

## Features

- Built with a modern 64-bit, C++23 toolchain
- Window resizing, HUD scaling, and widescreen support
- Replaced Win32 API calls with SDL3
- Controller support for Xbox, PlayStation and Switch pads
- No disc needed: support for playing music tracks from local files
- Skippable starting splash screens
- Various crash fixes for modern Windows
- A drop-in exe from GitHub Releases: no game files are needed to build it

## TODO

- Replace the DirectX hardware renderer with SDL3 renderers.
- Implement and refine texturing in both the software and hardware renderers.
- ImGui debug menus.
- Recreate the RC menus with ImGui.
- Recreate all the debug and plan menus with ImGui.
- Fix the existing bugs and balance issues in the PBN files ([docs/plan_bugs.md](docs/plan_bugs.md)).
- Builds and releases for: Windows, Mac, Linux, and Emscripten.
- Net and local multiplayer.
- Bot-only matches.
- A free-fly spectator mode.

## Install

Download `hyperblade_20XX.zip` from [Releases](https://github.com/zach2good/hyperblade-20XX/releases)
and unzip it into your HyperBlade folder, beside `HYPERX.EXE`. Then run `hyperblade_20XX.exe`.
Your install is left as it was.

The first run reads the game's text out of `HYPERX.EXE` into `hyperblade_20XX_strings.txt`, and
later runs load that file. Edit it to change or translate the text, or delete it to read it again.

The music was the disc's audio tracks. Rip them to `Data/Music/Track02.flac` and so on, as FLAC, MP3
or WAV. The menu plays track 10.

## Build

You need Windows, Visual Studio 2022 or later with the C++ x64 tools, CMake 3.21 or later and
Ninja. The build uses nothing from the game. From an x64 Developer Command Prompt:

```sh
cmake --preset msvc-x64 -DHYPERX_GAME_DATA=C:/path/to/your/HyperBlade
cmake --build build
```

`HYPERX_GAME_DATA` is optional. Point it at your HyperBlade folder, and the build links its `Data`
folder and copies its `HYPERX.EXE` beside `build/out/hyperblade_20XX.exe`, which you can then run.
`ctest --test-dir build` checks the string and INI files, and with the game it also decodes every
MDM1-packed file and parses every wave.

The tools below need Python 3.12 or later, and some read your `HYPERX.EXE`: copy
`hyperx-user.example.yml` to `hyperx-user.yml` and point it there.

Format every source file you touch with clang-format 22: `pip install clang-format==22.1.8`.

## Pulling from the decomp

The decomp is the `upstream` remote. Rebase onto it with `git fetch upstream` and
`git rebase upstream/main`. Its matching tools, records and CI are deleted here, so resolve a
conflict on any of them by deleting the file again.

## Tools

```sh
python tools/extract_rc.py          # menu, dialogs, strings, version and icon, as an .rc script
python tools/extract_mdm1.py --png  # every MDM1-packed bitmap in Data, as BMP and PNG
python tools/plan_dump.py           # the AI plans, printed as pseudo-source
```

[docs/](docs/README.md) holds what we know about the game: the class hierarchy, the other files on
the disc, and the bugs in the shipped AI plans.

## Legal

This project is for preservation, research and interoperability. It holds no original binary,
game data or disassembly. Don't attach any of these to an issue or a pull request, and don't ask
for a copy of the game.

LGPL-3.0-only for this project's own work. [NOTICE](NOTICE) says what the licence covers.
HyperBlade belongs to its rights holders.
