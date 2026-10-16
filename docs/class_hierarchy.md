# The class hierarchy, read from the vtables

The stripped image has no type information, so its vtables are the only evidence of inheritance.
VC6 lays out a derived class's table as a copy of the base's table with the overridden slots
replaced. Two tables that agree on a prefix of slots share a base, and the length of the prefix
is the base's virtual count. Slot 0 is always the class's own deleting destructor, so it never
agrees.

A table's length is the distance to the next table. A reader that uses a fixed slot count walks
into the next table and makes every class appear to override every slot.

Each class in `src/` that owns a table carries a `// VTABLE: HYPERX 0x...` marker. To list them,
run `grep -rn "VTABLE: HYPERX" src`.

## The game object family

Fifteen 32-slot tables agree on slots 1 to 25:

| Table | Class | Header |
|---|---|---|
| 0x004b38f0 | `EngineObject` | `src/engine/engineobject.h` |
| 0x004b286c | `CameraObject` | `src/engine/cameraobject.h` |
| 0x004b3464 | `DeviceObject` | `src/input/deviceobject.h` |
| 0x004b34e4 | `KeyboardDeviceObj` | `src/input/keyboarddeviceobj.h` |
| 0x004b3564 | `GamepadDeviceObj` | `src/engine/gamepaddeviceobj.h` |
| 0x004b35e4 | `JoystickDeviceObj` | `src/engine/joystickdeviceobj.h` |
| 0x004b3664 | `MouseDeviceObj` | `src/engine/mousedeviceobj.h` |
| 0x004b39f4 | `RokObject` | `src/engine/rokobject.h` |
| 0x004b3a74 | `WeaponObject` | `src/engine/weaponobject.h` |
| 0x004b3af4 | `PlayerObject` | `src/engine/playerobject.h` |
| 0x004b3b74 | `GoalObject` | `src/engine/goalobject.h` |
| 0x004b3bf4 | `DromeObject` | `src/engine/dromeobject.h` |
| 0x004b3c88 | `ParticleSystem` | `src/engine/particlesystem.h` |
| 0x004b3d0c | `CoachObject` | `src/engine/coachobject.h` |
| 0x004b3e64 | `RampObject` | `src/engine/rampobject.h` |

`ObjectBase`'s table at 0x004b3970 has 26 slots: a destructor and 25 `HandlePureVirtualCall`
entries. That is what VC6 emits for pure virtuals, so the original declared `ObjectBase` with 25
pure virtuals. `EngineObject` implements all 25 and adds six, for 32. Only `RokObject` and
`PlayerObject` override anything: slots 25 and 26, and `PlayerObject` also slot 31.

| Slot | Name | Slot | Name |
|---|---|---|---|
| 1 | `GetSlot` | 17 | `InstantiateAIPlan` |
| 2 | `GetPhysicsBody` | 18 | `GetPhase` |
| 3 | `GetLocalTransformPtr` | 19 | `EnqueueSceneEvent` |
| 4 | `GetObserver` | 20 | `FindPlanByName` |
| 5 | `SetObserver` | 21 | `GetWorldPos` |
| 6 | `PostSceneMessage` | 22 | `GetPositionPtr` |
| 7 | `PostEvent` | 23 | `IsVisible` |
| 8 | `PostSceneEvent` | 24 | `SetVisible` |
| 9 | `DispatchSceneMessage` | 25 | `ApplyRenderSlotPose` |
| 10 | `StartClipPlayback` | 26 | `Postpass` |
| 11 | `BindAnimEventChannel` | 27 | `SetEntitySlot` |
| 12 | `ResetSceneMessageHandlers` | 28 | `DrawTransformedMesh` |
| 13 | `GetPlanState` | 29 | `GetField04c` |
| 14 | `RegisterSceneMessageHandler` | 30 | `GetComponentCount` |
| 15 | `GetPhysStateObj` | 31 | `UpdateRenderSlotTransform` |
| 16 | `GetCPlayerStateLock` | | |

`src/engine/objectbase.h` and `src/engine/engineobject.h` are the authority for these names.

Slot 1's name misleads. It returns the low word of the object's slot index at 0x18, whose low
bit is the team. Every class in the family shares the body at 0x00422f30, so it identifies no
class. The object's kind is the field at 0x20.

The names were settled by slot index and arity. The small per-file classes they replaced used
one name for different slots, so matching by name would have merged the wrong slots.

## Other families

`python tools/vtable_forest.py` prints the full list. The main groups:

- **TransformObject.** `TransformObject` (0x004b223c) has 15 slots.
  `CameraProjectionState` (0x004b28f4) derives from it and has 27: the transform's 15 and 12 of
  its own. Its last four point at the same bodies as the transform's rotate and subtract, which
  are not slots on the base.
- **Animation binding.** `AnimBinding` (0x004b29b4) has 16 slots, 14 of them pure.
  `AnimPlayer` (0x004b2974) derives from it and implements all 16.
- **Players.** `PlayerBase` (0x004b2b2c) and `CPlayer` (0x004b2b80) have 11 slots. Slot 10 is
  pure in `PlayerBase` and `CPlayer::SendPacket` in `CPlayer`.
- **Surfaces.** `CDDBitMap`, `ScrollBitmap`, `FramedBarBitmap` and `DamageBar` (0x004b2e2c to
  0x004b2f84) share a 4-slot base. `Texture` (0x004b2d5c) and the radar (0x004b30d4) share an
  8-slot base.
- **Renderers.** `HUD` (0x004b3004) and `InfoBar` (0x004b32ec) both derive from `InfoBarBase`
  (0x004b3068), which derives from `Renderer`.
- **Screens.** Four screen tables (0x004b22e0, 0x004b2350, 0x004b3370, 0x004b33e4) share a
  3-slot base: `AwardsUIScreen`, `MatchInfoUIScreen`, `LeagueSetupUIScreen` and
  `ChampionshipSetupUIScreen`.
- **Widgets.** Five tables from 0x004b2a68 (`Widget`) to 0x004b2af4 share a 3-slot base.
  `LoopFlipbookWidget` (0x004b2abc) and `DisplayWidget` (0x004b2b10) share a 6-slot base.
- **Pools.** 35 agenda item pool tables (0x004b17c4 to 0x004b1a6c) and 25 behaviour pool tables
  (0x004b23e8 to 0x004b25c8) are 5-slot tables that share a 2-slot base. The pool
  owners, one header each under `src/common/`, look like duplicates to `tools/view_classes.py`,
  but each needs its own class.
- **Agenda items.** `AgendaItem` (`src/planvm/agendaitem.h`) has 13 slots, and 36 tables share
  eight of them. `AgendaItem::Activate`, slot 5 at 0x0040b990, calls slots 6, 8 and 9 on its own
  `this`.

## Abstract bases

VC6 filled some tables with `HandlePureVirtualCall`. Those are the classes the original declared with pure virtuals. The
counts below leave out slot 0, the destructor.

| Table | Class | Pure slots |
|---|---|---|
| 0x004b2218 | `AgendaItemOwner` | 4 of 4 |
| 0x004b2834 | `BehaviorOwner` | 4 of 4 |
| 0x004b2a2c | `BubbleNode` | 2 of 2 |
| 0x004b3840 | `NodeOwner` | 4 of 4 |
| 0x004b3970 | `ObjectBase` | 25 of 25 |
| 0x004b4098 | `AudioMgrBase` | 5 of 5 |
| 0x004b23c8 | `GameBehaviorObject` | 1 of 5 |
| 0x004b29b4 | `AnimBinding` | 14 of 15 |
| 0x004b2a68 | `Widget` | 3 of 6 |
| 0x004b2b2c | `PlayerBase` | 1 of 10 |
| 0x004b2b58 | `PaletteFade` | 3 of 4 |
| 0x004b2be0 | `CollisionShape` | 7 of 12 |
| 0x004b3068 | `InfoBarBase` | 23 of 24 |
| 0x004b32bc | `CDDDrawable` | 1 of 1 |
| 0x004b4040 | `Screen` | 1 of 2 |

Where the tree implements a slot that one of these tables fills with `HandlePureVirtualCall`,
the original left that slot to a derived class.

## Slots shared across unrelated tables

The linker folded identical bodies onto one address, so some slot bodies appear in many
unrelated tables and cannot be one class's member. `Object::Trace` is in seventeen tables,
`DispatchSceneMessage` in twelve, `CreateSurface` in seven and `KeyConfigDialog::SaveKeysAs` in five.

## COM interfaces

The game calls DirectDraw, Direct3D, DirectSound and DirectPlay through the interfaces in the
compiler's own SDK headers: `IDirectDraw`, `IDirectDrawSurface`, `IDirect3DDevice`,
`IDirect3DViewport`, `IDirectSound`, `IDirectSoundBuffer` and `IDirectPlay`. Each call's slot
was checked against that interface, so a method name is the slot the original calls.

## Open questions

- Slots 6 to 9 of `Interactor` have competing readings. Slots 7 and 8 return a
  collision object id that indexes `GameEngine::m_ppColObjects`. Which is the actor's own and
  which the other party's is not known.
- Byte 0x75 has two readings: `m_bHasRow3` on the transform and a perspective flag on the camera
  projection. The tree uses the transform's name, and the field records the disagreement.
