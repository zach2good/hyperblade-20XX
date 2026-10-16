# hyperblade-20XX

<p align="center">
  <img src="docs/logo.png" alt="HyperBlade" width="320">
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

## Build

You need Windows, Visual Studio 2022 or later with the C++ x64 tools, CMake 3.21 or later, Ninja,
Python 3.12 or later, and HyperBlade with `HYPERX.EXE` at v1.0.0.1. The build reads the game's
resources out of your copy.

Copy `hyperx-user.example.yml` to `hyperx-user.yml` and point it at your `HYPERX.EXE`. Then, from
an x64 Developer Command Prompt:

```sh
cmake --preset msvc-x64 -DHYPERX_GAME_DATA=C:/path/to/your/HyperBlade
cmake --build build
```

`HYPERX_GAME_DATA` is the folder that holds the disc's `Data` folder. The build links it beside
`build/out/hyperx.exe`, which you can then run. `ctest --test-dir build` decodes every MDM1-packed
file in your data. Without the game, configure with `-DHYPERX_COMPILE_ONLY=ON` to compile only.

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

## TODO

- Replace Win32 calls with SDL3, including SDL_Renderer for the software renderer.
- Replace the raw DirectX hardware renderer with SDL3 renderers.
- Implement and refine texturing in both the software and hardware renderers.
- ImGui debug menus.
- Recreate the RC menus with ImGui.
- Recreate all the debug and plan menus with ImGui.
- Fix the existing bugs and balance issues in the PBN files ([docs/plan_bugs.md](docs/plan_bugs.md)).
- Controller support through SDL3.
- An Emscripten build.
- Test on Mac and Linux.
- A resource-free executable we can ship, which runs when dropped into a real HyperBlade install
  with no other steps. CI builds it and publishes it with GitHub releases.

## Legal

This project is for preservation, research and interoperability. It holds no original binary,
game data or disassembly. Don't attach any of these to an issue or a pull request, and don't ask
for a copy of the game.

LGPL-3.0-only for this project's own work. [NOTICE](NOTICE) says what the licence covers.
HyperBlade belongs to its rights holders.
