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

## The compilers this project does not have

`HYPERX.EXE` was linked with LINK 6.00 and has a Rich header. No other file on the disc has one.
VC6 introduced the Rich header, so its absence is evidence of an older toolchain.

| Module | Linker | Product it implies | Rich header |
|---|---|---|---|
| `HYPERX.EXE` | 6.00 | Visual C++ 6.0 SP3 | 12 entries |
| `MIDIDLL.DLL` | 2.50 | Visual C++ 2.x | none |
| `GET3D.DLL`, `VIDDLL.DLL`, `SPLASH.EXE`, `MOVIE.EXE` | 3.00 | Visual C++ 4.0 | none |
| every DirectX DLL | 3.10 | Visual C++ 4.1 or 4.2 | none |

The product column uses the standard mapping from linker version to product, and is inference.
The timestamps agree with it, but these files state no compiler version the way `@comp.id` does
in a VC6 object. For `HYPERX` the toolchain is provable. For these, a match of the generated code
would have to establish it.

Our method assumes VC6. `cmake/toolchain-vc6sp3.cmake` takes the VC6 tree from
`VC6_ROOT` and stops configuration when the tree lacks one of its tools, and
`verify-toolchain` compares Rich header stamps and fails the build on a difference. A DLL target
needs a second compiler tree, a second toolchain file, and no `verify-toolchain` step, because
there is no Rich header to compare. CMake no longer supports Visual C++ below 6.0, so a VC 2.x or
VC 4.x target needs a hand-written NMAKE makefile driven from CMake.

## Two obstacles to byte-exact DLLs

Both apply to `VIDDLL` and `MIDIDLL`. Neither applies to `HYPERX`.

**Incremental linking.** The thunk table shows `/INCREMENTAL:YES`. The game executable was linked
`/INCREMENTAL:NO /OPT:REF`, so its layout follows from its objects. An incremental link leaves
padding for functions to grow into. The amount depends on the `.ilk` file, which depends on the
sequence of builds the developer ran. A clean incremental link is reproducible. The one that
shipped may not be.

**The CodeView record.** A byte-exact file includes the debug directory. Each `NB10` record holds
a signature and a path:

| Module | Signature | Age | Path |
|---|---|---|---|
| `VIDDLL.DLL` | `0x31f69d04`, 1996-07-24 22:00:36 UTC | 0 | `D:\vidplay\viddll\Debug\viddll.pdb` |
| `MIDIDLL.DLL` | `0x30a3cccc`, 1995-11-10 22:06:04 UTC | 1 | `C:\splash\WinDebug\mididll.pdb` |

The linker writes the signature as a build timestamp. The path is absolute. `/PDB:` can set the
path if `subst` maps the drive, but nothing can set the timestamp. A complete match therefore
needs a post-link patch. That's a weaker claim than "the build reproduces the bytes", and any
result must say so.

`GET3D.DLL` has no debug directory and no thunk table.

## What reccmp would need

reccmp supports several targets: `RecCmpProject.targets` is a dictionary, `asmcmp --target`
selects one, and markers already carry the target name (`// FUNCTION: HYPERX 0x...`). A `GET3D`
target needs:

1. A `GET3D` entry in `reccmp-project.yml` with `filename`, `source-root` and `hash`.
2. A `GET3D` entry in `reccmp-build.yml` that points at the built DLL and its PDB.
3. A `.def` file, so the export gets the right name and ordinal.
4. A second object library in `CMakeLists.txt`, linked twice like `hyperx` and `hyperx_sym`. The
   `/OPT:NOREF` reason for the sym build applies unchanged.
5. `verify-toolchain` limited to the `HYPERX` target. `pecompare.py` works on any PE file.

## Ranking

| Target | Own code | Verdict |
|---|---|---|
| `GET3D.DLL` | about 700 bytes | The only one worth doing. One export, no debug directory, no thunks, no assembly. It needs a VC 4.0 tree and a build that can drive it. |
| `MIDIDLL.DLL` | about 780 bytes | Small, but VC 2.x, incremental thunks and a CodeView timestamp no build can reproduce. Reverse it for the record. Do not promise the bytes. |
| `VIDDLL.DLL` | about 2,240 bytes | The same obstacles, and a statically linked debug CRT that must match before the team's code can. |
| Eighteen DirectX DLLs | none | Out of scope. |
| `SPLASH.EXE`, `MOVIE.EXE` | not counted | Useful only together with the three DLLs. Same VC 4.0 obstacle. |

## Recommendation

Do not open a second target while `HYPERX.EXE` is not fully matched. `python tools/progress.py`
gives the current figure.

If the DLLs are wanted, do these steps in order and stop at the first that fails:

1. Reverse `GET3D.DLL` in Ghidra as a plain decode, with no build. It is about 700 bytes, and it
   is the only one of the three that decides how the game starts: it holds the front end's
   hardware-detection rule.
2. Find out if a VC 4.0 tree can be obtained and driven from NMAKE. Until it can, no DLL target
   can be byte-exact.
3. Add the reccmp target.

## Open question

`GET3D.DLL` holds the string `IsTNT` in `.data` at file offset `0x2a4c`, immediately before
`kernel32.dll`. The file is dated November 1996 and the RIVA TNT shipped in 1998. Either the
string means something else or the timestamp is wrong. A decode of the 700 bytes of code would
answer it. Nothing else here depends on the answer.

## Where the figures come from

No single command reproduces this survey. `tools/peinfo.py` is the shared PE reader library, and
`tools/richcheck.py` and `tools/pecompare.py` are the PE tools with a command line. Each figure
above comes from one of these fields:

| Claim | Where it is read |
|---|---|
| Linker version | `MajorLinkerVersion`, `MinorLinkerVersion` in the optional header |
| Section names, `CSEG`/`DSEG` | Section table |
| Rich header | Between the DOS stub and the `PE\0\0` signature, XOR-keyed, ended by `Rich` |
| Exports, imports | Data directories 0 and 1 |
| `NB10` signature and path | Data directory 6, third entry, at its `PointerToRawData` |
| Own code boundary | `AddressOfEntryPoint`, the CRT startup |
| Function-shaped regions | Runs of `0xcc` in `.text` |
| Version resource strings | UTF-16 text in `.rsrc` |
