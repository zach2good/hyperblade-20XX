# Corrections to the Ghidra database

A Ghidra name is a guess. This file lists the names, signatures, extents and comments in the
Ghidra database that we've found to be wrong. Each entry gives the address, the Ghidra reading,
what the code is, and the evidence. Without this record, the same name gets trusted again on
sight.

The `name` column of `symbols/functions.csv` holds Ghidra's reading, as `tools/pull_functions.py`
pulled it, and isn't corrected there. The corrected name is in the tree's marker, which is the
name reccmp uses. `tools/publish_ghidra.py` pushes the tree's names into your own Ghidra project,
as [GHIDRA.md](GHIDRA.md#pushing-to-ghidra) says.

## Names that describe the wrong thing

### 0x00450160: the Rok value, not a timer

Ghidra: `InfoBar__UpdateTimerDisplay`. Tree: `InfoBar::UpdateRokValue`.

The format string at 0x004c0d6c is `"x%d"`, and the value is `abs(RokObject::m_wRokValue)` at
slot+0xd24, the same figure `src/ui/hud.cpp` draws into `g_hudSurfaces.pSmallText`, which starts as "x4".
The global it caches through, 0x004cef54, is a second last-value cache beside the HUD's
`g_nLastRokValue`, so the two displays reformat independently. The tree names it
`g_nInfoBarLastRokValue`.

### 0x00412550: the pass bar agenda item's Process

Ghidra: `FUN_00412550`. Tree: `CDisplayPassBar::Process`.

- `PlanVM__InitializeInteractorPoolAF` installs its vtable, 0x004b2148, and the function is at
  +0x04, the slot this family uses for Process.
- The gate compares the rok carrier at entity slot 6's state block +0x5c with the watched player
  at `g_pGameEngine+0x50`.
- The second entity is `(n + 2) & 3` from the carrier, which on the slot map is the carrier's
  team mate.
- The result is `(1 - dot) * 3.4142139 * (overlayPos.y >> 1)`. 3.4142139 is 1 / (1 - cos 45), so
  the marker is centred when the mate is straight ahead and at the screen edge at 45 degrees.

### 0x0043af20: the DirectDraw failure exit, not initialisation

Ghidra: `dword __stdcall InitializeDirectDraw(void)`. Tree: `int __cdecl
DDraw::FailInitialization(int nError)`, in `src/graphics/ddraw.cpp`.

It releases every DirectDraw surface and calls `GameApp::Fatal` with
`"DirectDraw initialization failed err=%d!"` from 0x004c075c. It does not return. Ghidra read
`__stdcall` from a bare `ret`, but all nine call sites push one dword and clean it up, and the
v1.0.0.0 FPO record counts one parameter.

### 0x00434f90: CreateVidTexture, not CreateAlphaSurface

Ghidra: `CD3DTexture__CreateAlphaSurface`. Tree: `CD3DTexture::CreateVidTexture`.

Its four diagnostics, at 0x004bdc7c, 0x004bdc4c, 0x004bdc20 and 0x004bdbf0, all name the
routine:

    Create surface in CreateVidTexture failed.
    CreatePalette in CreateVidTexture failed.
    SetPalette in CreateVidTexture failed.
    QueryInterface in CreateVidTexture failed.

No string in the image spells `CreateAlphaSurface`. The body takes the source surface's
description, sets the flags and capabilities a texture needs, creates a second surface, copies
the device palette onto it when the format is palettised, and queries it for the texture
interface. It does nothing with alpha.

### 0x00484730: a constructor, not a table initialiser

Ghidra: `GameConfig__InitParseTable`, with three stack dwords in the inventory. Tree:
`TeamBlock::TeamBlock()`.

Its one call site, at 0x00482ef8 in `ParseTeamHTMFile`, follows `operator new(0x888)` and its
null test with `MOV ECX,EAX` and `CALL`. It has no pushes and no cleanup.

### 0x0042a050: a constructor, not a function that returns its argument

Ghidra: `BoundBubble__InitCmd`. Tree: `BubbleCmd::BubbleCmd()`.

The caller, `GameEngine::Cmd_SetCollisionObject` at 0x00483300, opens with `PUSH -1` and
`PUSH __ehhandler`, sets a trylevel word to 0 before the call and to -1 after it, and reaches the
function through `PUSH 0x14`, `operator new`, `TEST EAX,EAX` and `MOV ECX,EAX`. Only `new T` on a
class with a constructor emits that.

The record is twenty bytes: kind and a flag word, a reference count and two bytes of padding,
then the next pointer, an extra block and a position block. The constructor writes kind 0x75 and
clears the rest.

### 0x0042a280: face normals, not a collision mesh reset

Ghidra: `Mesh__ComputeFaceNormals`. Tree: `CollisionCage::ComputeFaceNormals`.

The body computes the collision cage's world vertices, its bounds and its twelve face normals.
`EngineObject::SelectAndActivatePlan` calls it when the object has a bound bubble with a mesh.

### The four team gate functions answer the opposite of their names

Ghidra: `GameEngine__IsTeam1AboveMinSpeed` (0x00481620), `IsTeam2AboveMinSpeed` (0x00481820),
`IsAnyEntityAboveThreshold` (0x004817a0) and `IsTeam2AnyEntityAboveThreshold` (0x004819a0). Tree:
`GameEngine::IsHomeTeamInjured`, `IsAwayTeamInjured`, `IsHomeTeamDead` and `IsAwayTeamDead`.

All four return 1 when no entity on the team is over the gate, and 0 as soon as one is: `MOV
BL,1` in the prologue, a walk over the team's four slots, and `XOR AL,AL` in a second epilogue
when a value is above the gate. The value is `STAT_DAMAGE`, not a speed. The first pair gates
it at 3, 6 or 9 by difficulty and is what `PlanCond::TeamIsInjured` asks; the second gates it
at zero and is what `PlanCond::TeamIsDead` asks. `ENUMIDS.H` names both plan conditions, and
the next one, `MyTeamIsInjured`.

The same misreading named four more functions for a speed:

| Address | Ghidra | Tree | What it answers |
|---|---|---|---|
| 0x00481590 | `GameEngine__AreAllPlayersAboveMinSpeed` | `GameEngine::AreAllPlayersInjured` | 1 when no player's `STAT_DAMAGE` is above the difficulty gate |
| 0x004284c0 | `GameEngine__AnyPlayerSlowed` | `GameEngine::IsAnyPlayerInjured` | 1 when any player's is at or under it |
| 0x00486030 | `GameEngine__IsSlotAboveMinSpeed` | `GameEngine::IsFitSubstitute` | 1 for a bench row, neither seated nor already chosen, above it |
| 0x004626d0 | `PlanCond::IsOwnTeamAboveMinSpeed` | `PlanCond::MyTeamIsInjured` | the actor's own team's `IsHomeTeamInjured` or `IsAwayTeamInjured` |

### 0x00422750: rotation around X, not Z

Ghidra's comment on `Camera__InitViewTransforms` (tree: `CameraObject::InitViewTransforms`) says
it rotates with `RotateAroundZ`. The call at 0x004229a7 goes to 0x00427940, which Ghidra names
`RotateAroundX_Actual` and the tree writes as `TransformObject::RotateXStaged`. The sine and
cosine go to fields 0x64 and 0x68, the staged pair that function reads.

The same comment's `camera+0xFD0 (view_matrix)` and `camera+0x1010 (fov_h)` are right. The mirror
flag at 0x004d0a2c is the tree's `g_bBottomUpDIB`.

### 0x0049fe20: the compiler's pow helper

Ghidra: `CRT__PowCore`. Tree: `__CIpow`, a `LIBRARY` marker in `src/common/library.h`.

`SWPalette::LookupColorIntensity` at 0x004263a0 and one other reader load two values on the x87
stack and call this 20-byte function: `sub esp,0x10`, `fxch`, `fstp [esp]`, `fst [esp+8]`, a call
to the 174-byte body at 0x0049fe42, `add esp,0x10`, `ret`. It has the same shape as `__CIacos`
and `__CIasin` at 0x0049ebb0 and 0x0049f1f0. A `pow()` call in the source emits it. Ghidra's
`CRT__PowCompute` for the body at 0x0049fe42 is correct.

### 0x0049f67e: the vector destructor iterator

VC6 emits a call to this routine for `delete[]` on a class with a destructor. It computes
`pArray + nSize * nCount`, then loops while `--nCount >= 0`, subtracts the stride before each
call, and destroys the array from the last element to the first with the element in ECX. It is
LIBCMT's `??_M@YGXPAXIHP6EX0@Z@Z`, and `src/common/library.h` pairs it by that name.

### 0x00407150, 0x00407360, 0x004073a0: the splash dialog's handlers

Ghidra: `AboutDlg::OnInitDialog`, `AboutDlg::OnCommand` and `AboutDlg::PaintSplashBitmap`. Tree:
`SplashDialog::OnInitDialog`, `SplashDialog::OnCommand` and `SplashDialog::PaintSplashBitmap`.

`SplashDialog::Proc` at 0x004070b0 calls all three, nothing else in the tree does, and they sit
directly after it in the same unit.

### 0x0043d8d0 and 0x004310a0: two names that carry their address

- 0x0043d8d0, `HUD::LayoutSurfaces_d8d0`, is `HUD::PlaceSurfaces`. `HUD::LayoutSurfaces`
  (0x0043e140, slot 2) only calls it, and a static member cannot share the virtual's name.
- 0x004310a0, `D3DRend::InitState_10a0`, is `D3DRend::ResetState`. It only jumps to
  `InitRenderState` (0x00431970), and its one caller is `DDraw::ClearDeviceState` (0x0043ade0).

### The renderer's head cam, reticle, pass bar and quit prompt slots

The HUD bodies at 0x0043ef20..0x0043f320 and the InfoBar bodies at 0x00451ed0..0x00452120 fill
the same renderer slots. Ghidra names them for guesses about dialogs, scenes, overlays and timers.

| Address | Ghidra | Tree |
|---|---|---|
| 0x0043ef20 | `OverlayRaiser__ShowDialog` | `HUD::ShowHeadCam` |
| 0x0043ef40 | `Accessory__SetMessageText` | `HUD::SetHeadCamText` |
| 0x0043ef90 | `Scene__GetTargetPointClamped` | `HUD::GetHeadCamSize` |
| 0x0043f010 | `HUD::GetOriginPoint` | `HUD::GetHeadCamOrigin` |
| 0x0043f0b0 | `HUD::ForwardPointToScene` | `HUD::SetReticlePos` |
| 0x0043f130 | `Scene__GetTargetPoint` | `HUD::GetReticleSize` |
| 0x0043f220 | `HUD::GetOverlayPos` | `HUD::GetPassBarSize` |
| 0x0043f280 | `HUD::DrawOverlayAt` | `HUD::SetPassBarMarker` |
| 0x0043f2b0 | `HUD::ShowOverlay` | `HUD::ShowPassBar` |
| 0x0043f2d0 | `HUD::ShowMessagePair` | `HUD::ShowQuitPrompt` |
| 0x0043f320 | `TimerOwner__ResetTimers` | `HUD::HideQuitPrompt` |
| 0x00451f60 | `InfoBarText__SetFooter` | `InfoBar::SetHeadCamText` |
| 0x00451fb0 | `MetricList__GetNextCorner` | `InfoBar::GetHeadCamOrigin` |
| 0x004520d0 | `HudPanel__Slot07` | `InfoBar::GetPassBarSize` |
| 0x00452100 | `HudPanel__Slot10` | `InfoBar::ShowQuitPrompt` |
| 0x004501f0 | `InfoBar__UpdateSpeedBar` | `InfoBar::UpdateDamageBar` |
| 0x00422450 | `Camera::SetRenderMode` | `CameraObject::SetReticleState` |

- `HUD::CreateSurfaces` names the HUD surfaces in its error strings: "second camera" is the head
  cam frame and "camera reticle icon" is the reticle.
- The plan action that calls slot 12 is `id_ACF_SetHeadCamText` in ENUMIDS.H.
- Slot 19's modes are `Reticle_State` in ENUMIDS.H: `RET_Off`, `RET_Aim`, `RET_Commit`, `RET_Curve`.
  `CameraObject::SetReticleState` stores the mode and hands it to slot 19.
- Slot 14 is the head cam's origin in the back buffer: InfoBar answers the "frshot" frame's
  corner and HUD answers 0,0. It is used only as the head cam viewport's origin.
- Slots 20 to 22 have one caller, the `id_AGD_CDisplayPassBar` agenda item.
- The quit prompt is strings 159, "'Enter' to exit match", and 160, "'Esc' to continue".
  `GameApp::HandleGlobalKeyEvent` shows it on Esc in a live match and hides it on Esc or Enter.
- `InfoBar::UpdateDamageBar` is called for the four roster damage strips, with slots 0 to 3.

### The plan handlers take the names the plans use

The action and condition tables are indexed by `ActionFunctionIDs` and `DecisionFunctionIDs` in
`DATA/ENUMIDS.H`. The fifty-five handlers registered at one id each take the id's name in place of
Ghidra's, so `CheckTackleOpponentIfFacing` is `Engage_Opponent` and `GetRokCarrierState` is
`IHaveRok`. The handlers that keep another name are shared by several ids,
are `Stub::Assert`, live in `AnimEvent` with callers of their own, or, as `SendMsg`, would
collide with a `windows.h` macro.

### The behaviour-parameter handlers take the names the plans use

`g_apfnBehaviorParamHandlers` is indexed by `BehaviorSetParamFnIDs` in `DATA/ENUMIDS.H`. Each
handler takes its id's name, as the plan handlers do, in place of a name for what Ghidra saw it
write. `SetShortAtPlus72` is `SetInteractorLatency`, `SetByteAtPlus74` is
`SetDigressive` and `SetVec4AtPlus50` is `SetAnimationEvent`; the other eleven follow the same
rule. `SetCameraTrackingSlot` is registered at ids 10 and 14, `TargetGoal` and `DoubleScore`, so it
keeps a name of its own. The fields the first two write are `GameBehaviorObject::m_wLatency`, the
poll interval in frames, and `m_bDigressive`; the tree called them `m_wSlotCount` and `m_bQuiet`.

## Classes Ghidra split or merged

### AnimClip and AnimNamedRef are two classes

The tree once declared `AnimClip` twice with two layouts. Both layouts are real, and they
describe two classes.

`AnimClip` is the twenty-byte object. `AnimClipSet::Load` (0x004172d0) sizes its array as
`n * 20 + 4`, gives the vector-new iterator the pair 0x00417910 and 0x004163e0, and walks the
result at stride 0x14 calling `AnimClip::Load`. 0x00417910 (`AnimClip::AnimClip`) stores the
vtable 0x004b2238 at offset 0. 0x004163e0 (`AnimClip::~AnimClip`) restores it. Slot 0 of that
vtable is the vector deleting destructor at 0x00417440.

| Offset | Content | Where it is read |
|---|---|---|
| 0x00 | vtable 0x004b2238 | stored by 0x00417910, restored by 0x004163e0 |
| 0x04 | segment, a short | zeroed by 0x00417910, filled by `AnimClip::Load` |
| 0x06 | frame, a short | zeroed by 0x00417910, filled by `AnimClip::Load` |
| 0x08 | packed block | freed by 0x004163e0, read by `AnimClip::GetBoneTransform` |
| 0x0c | translation block | freed by 0x004163e0, read by `AnimClip::GetBoneTransform` |
| 0x10 | format, 1 to 8 | switched on by 0x004163e0 and by `GetBoneTransform` |

Ghidra read 0x004163e0 and 0x00417440 as a plan condition expression destructor
(`PlanVM__CondExpr_Dtor`). Nothing plan-related refers to them.

The other layout, a `char*` at 0 and a short at 4 tested against 0x270f, is `AnimNamedRef`. The
call graph shows it:

- 0x00417880 (`AnimNamedRef::AnimNamedRef`) is the only writer of that layout.
  `AnimDB::AddClipEntry` at 0x00417784 calls it once and stores the result at `+0x20` of the
  36-byte clip entry record.
- 0x004178e0, Ghidra's `AnimClip__FreeDecompData`, is called from `AnimDB::~AnimDB` at 0x00417621
  and from 0x00417864, both through that `+0x20` field. It frees the name only while the
  identifier is still biased, so it is `AnimNamedRef::FreeName`.
- 0x004177c0, Ghidra's `AnimBinding__SyncCheck`, is called from `AnimRegistry::BindAnimSets` at
  0x00417209 with `this` set to a clip entry taken from the `+0x3c` array. It is
  `AnimClipEntry::SyncCheck`.

The reference's first field has two uses, which is why the 0x270f test exists: it holds the
copied name while the identifier is biased, and the resolved set afterwards.

### One name, AnimSet, covered four objects

Each allocation size names one object:

| Object | Size | Where the size is |
|---|---|---|
| the registry (`g_pAnimRegistry`) | 0x10 | `AnimRegistry`'s constructor, `src/anim/animregistry.h` |
| its cache buffer, 250 set slots | 1000 | the same constructor |
| the script table (`AnimScriptTable`) | 0x4c0 | the same constructor |
| a script database | 0x68 | `push 0x68` at 0x004185e0 |
| an animation set | 0x20 | `push 0x20` at 0x00417237 |

Offset 0x0c is the script-table pointer on the registry and the heap copy of the name on a set.
`AnimDB__LoadFromPath` at 0x00417000 passes the object at 0x0c as the parse target and the
registry as the manager.

`AnimSet::GetFrameRange` isn't animation code. Its object is `g_pColorPalette`, and it reads the
eleven `PaletteRange` pairs at 0x410 in `src/graphics/colorpalette.h`. It is
`ColorPalette::GetFrameRange`. Ranges 4 to 11 belong to the fade table, and range 4 holds two
cycle lengths instead of a range.

### CDDText and CDDText2 are one class

Ghidra gives one object two prefixes.

- 0x00451c60, Ghidra's `CDDText__InitBase`, installs vtable 0x004b32d4. Slot 0 of that table is
  0x0044fa20, Ghidra's `CDDText2__VDtor`.
- Slot 1 is 0x0044fa40, `CDDText::Draw`.
- The field layouts agree. The base initialiser zeroes three points at 0x04, 0x0c and 0x14,
  which are `CDDText`'s size, position and offset pairs. The first 0x1c bytes are the surface
  base.

The tree has one class, in `src/graphics/cddtext.h`.

### NodeList, InteractorNodeList and Spline are one class

Ghidra: `NodeList__Init` at 0x00492e90 and `InteractorNodeList__RemoveTail` at 0x00492ec0. Tree:
`Spline::Spline` and `Spline::~Spline`.

- 0x00492e90 allocates two 0x30-byte `SplineCurve`s and links them into a ring. `CTrackTraj`
  and `CSlideOnRail` store what it answers as the spline their updates walk.
- The initialisers from 0x00492f10 to 0x004931a0 fill the same two words, and the updates at
  0x00493240 and 0x00493320 step the first word along the ring.
- The functions from 0x00492e90 to 0x00493320 are contiguous, so they came from one unit.

The tree has one class, in `src/math/spline.h`.

### 0x00433ef0: slot 27 of the surface is Restore

Tree: `CD3DTexture::Restore`. The call through slot 27 of the DirectDraw surface takes no
arguments, the object is the destination surface at this+0xfc, and the message after it is
`"Could not restore destination texture surface"`. The slot is `Restore`, not `GetHandle`. The
seven-slot interface used elsewhere in the same class is the Direct3D texture, whose handle
getter is at slot 4.

## The entity vtable

Nineteen class views in the tree once gave the entity vtable's slots eight different names. The
tables settle them:

| Slot | Address | Ghidra name | Returns | Tree name |
|---|---|---|---|---|
| 1 | 0x00422f30 | `Object__GetTeamIndex` | `+0x18`, the object's slot | `EngineObject::GetSlot` |
| 2 | 0x00422ec0 | `Object__GetPhysicsBody` | `+0x224` | `EngineObject::GetPhysicsBody` |
| 3 | 0x00422ed0 | `Object__GetLocalTransformPtr` | `this + 0x114` | `EngineObject::GetLocalTransformPtr` |
| 13 | 0x00422f40 | `Object__GetActivePlanState` | `+0x228` | `EngineObject::GetPlanState` |
| 22 | 0x00422e60 | `Object__GetPositionPtr` | `body + 0x44` | `EngineObject::GetPositionPtr` |

0x00422f30 is at `+0x04` and 0x00422ec0 at `+0x08` of fifteen vtables. The entity table at
0x004b3464 has 0x00422f40 at `+0x34` and 0x00422e60 at `+0x58`.

The field at slot 1 is the object's slot index, not a team or a kind. `GameEngine__LoadTeamFromFile`
at 0x004858b0 gives the six players 0/2/4 and 1/3/5, and `GameEngine__InsertObjectSorted` at
0x00486c70 compares the field to find an object already in the draw list. Reads that compare it
with six compare against the player count. Every object class shares one address at slot 1; the
object's kind is the field at 0x20.

Slot 13 returns the plan state block at `+0x228`. Its `+0x04` is the name string and `+0x18` is
an int.

## Globals

### 0x004cd3b0: an array of viewport rectangles

Tree: `g_d3dState.aViewportRects[2]`.

`D3DViewport::Clear` (0x00432370) passes the address to slot 12 of
`IDirect3DViewport`, which is `Clear(dwCount, lpRects, dwFlags)`, with a count of 1.
`D3DViewport::Setup` at 0x004316f0 fills a `D3DRECT` at that address for each viewport, indexed by
a counter at 0x004cd3a4.

### 0x004cbd80: a field inside the driver record

`D3D::FindDeviceByFormat` at 0x00431a80 reads a flag byte at `[reg + 0x004cbd80]` with the
0x3b4 stride of the driver records. `src/graphics/d3ddeviceinit.h` bases `g_d3dState.aD3DDrivers` at
0x004cbcd8, where the enumeration callback writes the first field, so 0x004cbd80 is a field of
the same record.

### 0x004cdac0: a clipper, not a surface

Tree: `g_ddrawDevice.pDDrawClipper`, typed `IDirectDrawClipper*`.

In the windowed path of `DDraw::CreatePrimaryAndBackBuffer` at 0x0043af40, its address is the
second argument of `IDirectDraw::CreateClipper` (vtable+0x10). The object that comes back is
called through vtable+0x20, `IDirectDrawClipper::SetHWnd`, with 0 and `g_hWndGame`. It then goes
to the primary surface's `SetClipper` (vtable+0x70). Nothing else in the image touches it.

### 0x004b3f04, 0x004b3f0c and 0x004b3f14: format string pointers

Three of a run of eight `const char*` at 0x004b3f00, pointing into the pool at 0x004bb804. They
are `"%hd"`, `"%d"` and `"%f"`. The object definition readers push these pointers, not the
literals, so every scanf call there loads the format from memory. Tree: `g_pszPercentHd`,
`g_pszPercentD` and `g_pszPercentF`. The neighbours are 0x004b3f08 `"%x"`, 0x004b3f10 `"%s"` and
0x004b3f1c, a tab.

### The HUD surface globals

`symbols/hud_surfaces.csv` pairs each database name with the tree's. Some database names describe
the wrong thing:

- 0x004cd52c and 0x004cd530, `g_pHUD_TeamADamageSurface` and `g_pHUD_TeamBDamageSurface`, are the
  logo borders "frdamgA" and "frdamgB" (`g_hudSurfaces.pTeamAFrame`, `g_hudSurfaces.pTeamBFrame`). The error strings in
  `HUD::CreateSurfaces` say "team A logo borders" and "team B logo borders".
- 0x004cd534 and 0x004cd538, `g_pHUD_TeamALossSurface` and `g_pHUD_TeamBSeaSurface`, are the two
  team logos. Their default bitmaps "hudlos" and "hudsea" are Los Angeles and Seattle.
- 0x004cd560 and 0x004cd564, `g_pHUD_TimerText1` and `g_pHUD_TimerText2`, hold strings 159 and
  160, the quit prompt (`g_hudSurfaces.pExitMatchText`, `g_hudSurfaces.pContinueText`).
- 0x004cd590, `g_pHUD_ScoreTicker1`, and its neighbour hold the two team names
  (`g_hudSurfaces.apTeamNames`), the twin of `InfoBar::m_atTeamNames`.

## Signatures

### Folded stubs have no one signature

`Stub::Assert` (0x0043e130), `KeyConfigDialog::SaveKeysAs` (0x0042bcd0) and `KeyConfigDialog::ClearKeys`
(0x0046d700) are one, three and six bytes of `ret`. The linker folded several unrelated functions
onto each address. 0x0043e130 is reached as a vtable slot, as a call with no arguments and as a
one-argument `__cdecl`. 0x0042bcd0 and 0x0046d700 are each called from
`KeyConfigDialog::OnCommand` with `(HWND, KeyRow*)` and cleaned with `add esp, 8`, and each is
also a vtable slot that takes `this`. No arity check can settle them.

### 0x00475510 and 0x004757f0: dialog procedures Ghidra does not have

Tree: `GameSetup::SessionListDialogProc` and `GameSetup::SessionConnectDialogProc`.

Ghidra has no function over 0x00475510 to 0x00475c6f. Both procedures end `ret 0x10`. One handles
`WM_INITDIALOG`, `WM_COMMAND` and `WM_TIMER` for the session-list dialog and the other for the
player-setup dialog. They call the nine `NetSetup_DialogProc__*` handlers, each with `HWND`,
`0x111`, `WPARAM` and `LPARAM`, and clean with `add esp, 0x10`. Ghidra shows those calls as cross
references with no containing function, so a sweep over named functions misses them.

### 0x00439bb0: DDraw::GetCurrentDisplayMode takes one argument

Ghidra: `__stdcall(void)`. Its one caller, `DDraw::InitDeviceAndSurfaces` at 0x0043a7b3, pushes the
DirectDraw object and cleans it with the `add esp, 0xc` that also covers `DDraw::GetCaps`'s two
arguments. It is `__cdecl` with one parameter that the callee never reads.

### 0x0043ef90: the stack dword is a hidden return pointer

The v1.0.0.0 FPO record gives one parameter. The tree writes it as `HUD::GetHeadCamSize`, which
returns a `ViewExtent` by value, so the dword is the hidden return pointer. Its only reference is
data, so there is no call site to read.

### 0x00491de0: Game::UpdateState takes nothing

The propagated v1.0.0.0 pairing gives two stack dwords. `GameApp::MainUpdate` calls it at 0x0044c17f with
no pushes and no cleanup, so the pairing is wrong.

### Functions Ghidra left free that are class members

- `Game__Error` (0x00411be0), `Game__Fatal` (0x004017a0) and `Game__FatalViaStringWrapper`
  (0x0042f3b0) take the application object first and are variadic. A variadic member is
  `__cdecl` with `this` as the first stack word, so these are `GameApp::Error`, `GameApp::Fatal`
  and `GameApp::FatalString`.
- A `__fastcall` function whose only argument is the object gets it in ECX, which is a member
  call with no arguments. `NetworkPlayer__ProcessReceivedPacket` is
  `EngineObject::ProcessReceivedPacket`, `MDM__SkipTwoLines` is `PlanVM::SkipLine`,
  `Network__CreatePacketCarrier` is `GameSession::CreatePacketCarrier`, `AnimSet__Init` is
  `AnimClipSet::Init`, `Camera__InitViewTransforms` is `CameraObject::InitViewTransforms` and
  `Mesh__ComputeFaceNormals` is `CollisionCage::ComputeFaceNormals`.
- `Object__ComputeDistance`, `Object__GetApproachAngleSector`, `Object__IsRokCarrier` and
  `Object__ResolveRokScoreOnCollision` are `__cdecl` with the object pushed by every caller. They
  are static members of `EngineObject`.
- The `GameEngine__` and `Renderer__` functions reach their object through the global, not
  `this`, so they are static members with their original conventions. `Renderer__DrawScene` is
  `Renderer::DrawWholeScene`, because the class already has a virtual `DrawScene`.
  `GameEngine__UpdateCoachAI` is `EngineObject::UpdateCoachAI`.
- The `Game__` window and application functions reach nothing through `this`. They are static
  members of `GameApp`, including the dialog procedure, the new handler and the animation peek
  that the code takes by address.

## Function extents

Ghidra's extents and entry points are wrong at these rows. `symbols/functions.csv` carries the
corrected values. reccmp does not read that file, so a wrong size there affects only the tools
that do, such as `tools/progress.py` and `tools/ready_unwritten.py`. A wrong entry point matters
more: a marker placed from it makes reccmp read padding as the function's first bytes.

| Address | Ghidra | Correct |
|---|---|---|
| 0x00408fd0 `CTracking__Process` | ends at 0x00409f98, 4041 bytes | ends at 0x00409fbe, 4079 bytes |
| 0x0040ad68 `CWatch__Process` | starts on padding | starts at 0x0040ad70 |
| 0x0040b718 `CCircleTracking__Process` | starts on padding | starts at 0x0040b720 |
| 0x0047a838 `thunk_InitializeLightAttenuation` | starts on padding | starts at 0x0047a840 |
| 0x00482b00 `Cmd_SetMotionParams` | 851 bytes | 880 bytes |
| 0x00484070 `GameConfig__MapModifierToId` | 891 bytes | 1000 bytes |
| 0x004070b0 `SplashDialog__Proc` | no function | 154 bytes |
| 0x00417910 `AnimClip::AnimClip` | no function | 28 bytes |

VC6 puts a switch's jump table in `.text` directly after the function body, and Ghidra ends the
function at the body. Each new jump-table switch can add a row of this kind. Run
`tools/check_row_sizes.py` after writing one.

## Behaviour the code must reproduce

These entries record places where the tree once had the wrong program. Each is a fact about the
original that a later edit could break again.

- **0x004105a0, `CSetAttitude::Process`.** The spin cosine and sine go to transform
  `+0x6c` and `+0x70` (`m_rotZ.flCos`, `m_rotZ.flSin`), not `+0x74` and `+0x78`.
- **`CPlayer::DequeueNetRecord`.** The ring index is unsigned. The original wraps it with
  `XOR EDX,EDX` and `DIV`. A signed index gives `CDQ` and `IDIV`, which gives a different result
  for the top half of the range.
- **0x0042c380, `PlayerBase::NetUpdateObject`.** The entity kind is unsigned. The original stores
  it as a byte at `[esp + 0x1c]`, reads four bytes back and masks with `and ecx, 0xff`.
- **0x00411550, `Player::FindBungeeVictim`.** At 0x004118a4 the test is `FCOMP 0.0`,
  `FNSTSW AX`, `TEST AH,1`, `JZ`. C0 is set when ST0 is below the operand, so the fall-through at
  0x004118b5 is the `flDet < 0` arm: `FADD 9.0` then `FDIV 3.0`. The taken arm at 0x004118c8
  computes `9.0 - flDiff`, then the same divide.
- **0x0046ac10, `LoadAnimationLists`.** It fills 276 slots across the two halves of the plan
  dispatch array. `PlanAction::SubstitutePlayer` at 0x00464140 goes into both 0x004cf244 and
  0x004cf490, which are `g_pfnPlanOpcodeHandlers[112]` and `g_pfnPlanActionHandlers[118]`. VC6
  loads an address stored into both halves into a register, so the four register loads at
  0x0046af78 name the four functions that appear twice. `tools/table_content.py` checks the
  table slot by slot against the original.

## Known differences the source does not reproduce

At these functions the listing shows the original program, but the source keeps a different
spelling because the correct one matches fewer bytes under this compiler.

- **0x0046ffc0, `EngineObject::ComputeBoundingSphere`.** Both bone loops read the byte at
  bone+0xdc and test it with `shr cl, 1` and `test cl, 1`. That is VC6's read of a one-bit
  bitfield at bit 1, so the byte is a bitfield and bit 1 is a per-bone skip flag. In this build
  a bitfield declaration and the `>> 1 & 1` the source uses both fold to `test bl, cl` with the
  constant 2 in `BL`, so neither reproduces the original's shift.
- **0x00408fd0, `CTrack::Process`.** At 0x409766 the original computes three half products
  (`fld`, `fmul 0.5`) before it stores any of them, and it does the same at 0x40981b, 0x40985d
  and, in integer registers, at 0x4097a4. This build interleaves compute and store at each site,
  even where the source already uses a whole-structure assignment. Three named locals grow the
  frame by twelve bytes, and this frame has no room.
