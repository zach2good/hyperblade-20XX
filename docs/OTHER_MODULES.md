# The other modules on the disc

HyperBlade ships more than `HYPERX.EXE`. The disc's root holds 10 DLLs and 7 other EXEs, and
`DIRECTX/` holds the DirectX redistributable. Does any of them belong in this project, and what
would it cost?

The eighteen DLLs under `DIRECTX/` are Microsoft's DirectX 2 redistributable and are out of
scope. Three small DLLs are the game team's own code. They're easy to reverse, but they belong to the
CD front end rather than the game, and each was built with a compiler we don't have.

Every claim below can be checked against the files on the game's disc.

## Microsoft DirectX 2, May and June 1996

`DDRAW.DLL`, `DDRAW16.DLL`, `D3DIM.DLL`, `D3DRM.DLL`, `D3DXOF.DLL`, `D3DHALF.DLL`,
`D3DRGBF.DLL`, `D3DRAMPF.DLL`, `D3DRG8F/16F/24F/32F.DLL`, `D3DRM8F/16F/24F/32F.DLL`,
`DPLAY.DLL`, `DSOUND.DLL`.

Their version resources read `Microsoft Corporation`, `Microsoft Direct3D`,
`DirectX for Windows` and `Copyright Microsoft Corp. 1994-1996`. They sit under `DIRECTX/`, the
redistributable installer directory. `DIRECTX/DDRAW.DLL` has MD5
`5c164e46e267b6d3d97be684de4e148d`.

They're Microsoft's DirectX 2 runtime and have no effect on whether `HYPERX.EXE` matches. Two
more facts make them poor targets:

- `D3DRG8F/16F/24F/32F` and `D3DRM8F/16F/24F/32F` have `CSEG` and `DSEG` sections next to
  `.text`. Those are MASM segment names. The rasterizer inner loops for each bit depth are
  hand-written assembly, so a match needs MASM 6.x as well as a C compiler.
- The seventeen 32-bit ones were linked with LINK 3.10, not 6.00. `DDRAW16.DLL` is a 16-bit NE
  module.

### Dropping them leaves no hole

Both of the game's renderers are inside `HYPERX.EXE`.

`MeshBone::DispatchFace` at `0x0044be40` is the whole switch:

```cpp
if (g_ddrawDevice.bD3DActive != 0) { D3DRend::DrawFace(...); return; }
SWRend::DrawFace(...);
```

The software renderer is the game's own and is complete in the image: `SWRend::DrawFace`,
`SortFaces`, `ScanEdge`, `FillSpans`, `DrawLine`, `ClipPolyAgainstPlane`, `BlitRLESprite`,
`FillRect`, `SetBltDestRect`, `DrawFaceList` and `SoftTex::DrawTexPoly`. That is about 4.8 KB
of scanline rasterizer, most of it between `0x0044a200` and `0x0044b170`, with
`SoftRenderer::ResizeBackBuffer` and `Renderer::DrawScene` around it.
`Renderer::DrawSceneDispatch` at `0x0044bc80` selects the whole-scene path from
`g_cUseSoftRenderer`, which `-x` sets.

`HYPERX.EXE` also holds the Direct3D client: `D3D::EnumDrivers`, `D3DDevice::SelectDriverAndMode`,
`D3DViewport::Setup`, `D3DRend::SetupRenderStates`, `D3DRend::BuildAndSubmitExecuteBuffer`,
`D3DRend::DrawFace` and the `CD3DTexture` family. It has no triangle rasterizer for that path. It
fills execute buffers and gives them to Direct3D Immediate Mode, which sends them to a vendor HAL
driver or to Microsoft's emulation in `D3DHALF`, `D3DRGBF`, `D3DRAMPF` and the bit-depth plugins.

So three rasterizers were reachable at run time, and only one of them is the game's:

| Path | Rasterizer lives in | In scope here |
|---|---|---|
| no Direct3D device | `SWRend` inside `HYPERX.EXE` | yes |
| Direct3D, hardware | the display driver's HAL | no |
| Direct3D, emulated | `D3DRGBF`/`D3DRAMPF` and a bit-depth plugin | no |

`SPLASH.EXE` asks `Get3DHardware`, then starts `hyperx.exe`, `hyperx.exe -x` or
`hyperx.exe -x -m`. `GameApp::ParseCommandLine` at `0x00419420` is the patched v1.0.0.1 parser,
in which `-x` forces the software renderer. The patch's version block says it reversed the meaning
of `-x`, and `SPLASH.EXE` is a 1996 binary, so this build does not show what `-x` meant to the
shipping front end.

## The game team's own DLLs

Microsoft did not build these three modules, `SPLASH.EXE` or `MOVIE.EXE`.

| Module | Bytes | `.text` | Exports | Linked | PDB path in the file |
|---|---|---|---|---|---|
| `GET3D.DLL` | 17,920 | 9,216 | `Get3DHardware` | 1996-11-08 | none |
| `VIDDLL.DLL` | 78,336 | 58,368 | `OpenVideo`, `PlayVideo`, `StopVideo`, `CloseVideo`, `MovieIsPlaying` | 1996-07-24 | `D:\vidplay\viddll\Debug\viddll.pdb` |
| `MIDIDLL.DLL` | 20,480 | 8,704 | `OpenMIDI`, `PlayMIDI`, `StopMIDI`, `CloseMIDI` | 1995-11-10 | `C:\splash\WinDebug\mididll.pdb` |

The PDB paths come from the `NB10` CodeView record in each debug directory. `MIDIDLL` was built
inside the `splash` project tree, so it is the launcher's MIDI player, not the game's.

### The game loads none of them

```txt
HYPERX.EXE   imports KERNEL32 USER32 GDI32 WINMM VERSION DDRAW DSOUND DPLAYX
SPLASH.EXE   imports KERNEL32 USER32 GDI32 ADVAPI32 WINMM WSOCK32 MIDIDLL VIDDLL
MOVIE.EXE    imports KERNEL32 USER32 GDI32 WINMM VIDDLL
```

`HYPERX.EXE` does not import or name any of the three. The only DLL it opens by name at run time
is `mdmtex.dll`, which the disc does not ship. `Graphics::LoadMdmTexDLL` at `0x0044b6f0` loads it
only when the command line has the developer marker `dmandappro!`.

`SPLASH.EXE` loads `GET3D.DLL`. It builds the path from `%s\Get3D.dll` and calls
`GetProcAddress` for `Get3DHardware`. `SPLASH.EXE` is the CD autorun front end: `AUTORUN.INF`
says `open=splash.exe`, and its strings include `3Dfx Interactive DirectX II Driver`,
`hyperx.exe -x` and `hyperx.exe -x -m`. The front end asks `GET3D.DLL` what hardware is present,
then starts the game with the matching switch.

Adding these DLLs would change the project's goal from the game to the game and its CD front
end. That's a scope decision before it's a question of difficulty.

### How much code they hold

The exports of `VIDDLL` and `MIDIDLL` are 5 bytes apart in `.text` and each starts with `e9`.
They are `jmp rel32` thunks, so both DLLs were linked `/INCREMENTAL:YES`. The function bodies are
behind the thunks.

The linker puts library code after user code, so the entry point, the CRT startup, marks the end
of the team's own objects:

| Module | Own code, estimated | CRT and library | Function-shaped regions in `.text` |
|---|---|---|---|
| `GET3D.DLL` | `0x1000` to `0x12c0`, about 700 bytes | `0x12c0` to `0x3400` | about 73 |
| `MIDIDLL.DLL` | `0x1000` to `0x130e`, about 780 bytes | `0x130e` to `0x3200` | about 68 |
| `VIDDLL.DLL` | `0x1000` to `0x18c0`, about 2,240 bytes | `0x18c0` to `0xf400` | about 289 |

The region counts are runs of `0xcc` padding. They estimate function boundaries and are not a
symbol count. Most of each count is CRT. `VIDDLL` statically links the debug CRT: it contains
`dbgheap.c`, `dbgrpt.c`, `_flsbuf.c`, `output.c`, `vsprintf.c`, `handler.cpp` and the string
`Microsoft Visual C++ Debug Library`. About 56 KB of its 58 KB of `.text` is library.

The team's own code is about 3.7 KB across three modules, with ten exported functions.
