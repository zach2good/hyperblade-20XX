# RE cruft

Decompiler output compiles to the right bytes and reads like a memory map: a pointer cast to
`char*`, an offset, a cast back. Each restates a fact the class already knows. This document
lists the shapes to look for, the tool that counts each one, and what is left.

Removing cruft can move bytes either way, so treat each change like any other: build, rescore,
and run `python tools/check_regressions.py`. [MATCHING.md](MATCHING.md) covers the
measure loop and the spellings that keep VC6's output unchanged.

## The shapes

### The blob

```cpp
char* const pObj = (char*)this;
nState = *(short*)(pObj + OBJECT_STATE);
```

The class declares a field at that offset, so the body should name it. The tool depends on what
the offset hangs off:

| Base | Tool |
|---|---|
| `this`, through a local or at each site | `tools/self_fields.py` (expands composites, so 0x24 into a `Vec3` is `vel.y`) |
| `this`, exact type match only | `tools/field_names.py` |
| A `Class*` that is not `this` | `tools/param_fields.py` (public fields only for a non-member) |
| A typed global, `(char*)g_pGameEngine + OFFSET` | `tools/global_fields.py` (refuses a gap) |

A loop that steps a byte offset across parallel arrays is often the original's own induction
variable. Measure before you convert one.

### The alias

`char* const pObj = (char*)this;` or `T* const pX = this;` gives `this` a second name. Most go
with the blob. Some stay: a second name can change which register VC6 gives the value. Such an
alias carries a one-line comment above it.

### The view

A struct declared only to reach one member of an object the code never types. Type the pointer
as the real class and delete the view.

### The free function that is a member

A `__fastcall Class__Name(Class* pThis, int edx, ...)` is a member function. Make it one; the
dummy EDX parameter goes with it.

### The pun

`*(int*)&m_flX = 0` writes a float with an integer store. A typed float assignment compiles to the
same integer `MOV`, so the pun goes.

- A float's bit pattern read as an integer goes through `FloatBits`.
- A word read again as its bytes takes getters that shift it, not a union. The exception is a
  client that stores the value to its frame and reads a half back from memory. That is a union
  in memory, as `Anim::EulerToMatrix`'s `FloatWord` shows (0x00416c73 to 0x00416cc4).
- A pun on a pointer or an integer field is a fault.
- So is a parameter, field or global that every access reaches through one pun type: the
  declaration has the wrong type. Retype it and remove the puns.

### The name that means two types

Two classes that declare one member name with two types. Most are correct: `m_pNext` points at
its own list's node type. A pair is a fault when one object is read through both, and worth a look
when one side is a builtin pointer and the other a class pointer, such as `float*` beside `Vec3*`.

### The offset read

`*(float*)((char*)p + 0x44)` on a typed base. Convert it only when the written class covers the
offset. A read past the class's end, or stepped by a variable, needs a decode first.

### The int parameter holding a pointer

An `int` or `void*` parameter cast back to one pointer type in its own body. Retype the
parameter.

### The untyped member or global

A `void*` or `char*` cast to one type everywhere is a free retype. One cast to several types is
usually the truth. One nothing casts is either a string or undecoded.

### The global named for its address

`g_flInit004B14A0` says where the value lives and nothing about what it is. Read its readers and
writers, in the tree and in the original, and name it for what it holds. A constant that each
source file of the original kept its own copy of takes the value and the file it belongs to, such
as `g_flZeroCamera`. `tools/placeholder_globals.py` lists each one with its readers and writers,
and the battery fails on a new one.

### The function named for a place

`InitializeGlobal004CB370`, `FUN_0044b0a0`, `Slot06` and `Dialog025C` say where a function sits,
in the image, in a table or in the resources, and nothing about what it does. Name a static
initialiser for what it initialises, a slot for what its overrides do, and a dialog procedure for
its dialog. A slot that nothing calls and that no override gives a meaning is `Unused`, with one
comment line saying so. `tools/placeholder_functions.py` lists each one with its callers and what
it touches, and the battery fails on a new one.

### The cast that is really a conversion

`(float)nValue` is a conversion and stays. `(Class*)pSame` on a pointer already of that type,
`((Base*)this)->Base::F()` and `(char*)p != 0` are noise.

### The constructor that assigns in its body

Assignments to members at the top of a constructor belong in the initializer list, except where
the original's store order needs the body.

### The Name__Function free function

`Bot__ReleaseScratchTransform`, `CRT__GetSystemTimeWrapper`: a class or namespace spelled into the
name. It becomes a member of that class, a function in that namespace, or, for library code, the
routine's real name. Bring-up code lives in `namespace BringUp`. `tools/check_prefixed_functions.py`
fails on any that comes back.

### The scattered family

A class or namespace whose bodies sit in many files, or in a file named for something else.
`tools/namespace_spread.py` counts both, marked bodies or not, and fails if either figure rises.
The bodies fold into the family's own file; a real exception goes in
`symbols/layout_exceptions.csv`. It also counts the bodies at global scope and fails if that
figure rises: a game function is a member of the class whose data it works on, or a function in
a namespace named for what it is, never a free function or a namespace that labels one function.

### The scattered record

Globals declared one by one that the original reads as one record: one function takes the first
one's address and reaches the rest at fixed offsets, or clears the run with one `memset`. The
markers keep each name's code matching, so the score cannot see the fault; the linker places the
names apart and the reader writes over whatever it put there. Declare one struct with the marker
on it. `tools/scattered_records.py` follows each marked address through the original; a row that
is not a record goes in `symbols/scattered_records.csv`.

### The note on the wrong body

A sort or a fold moves a body and leaves its note behind. The paragraph above a function then
describes a neighbour, a body in another file, or nothing at all at the end of a file.
`tools/misplaced_notes.py` scores each note against every marked body and lists the ones another
body holds more of, with three likely homes. About half of what it lists is real. A note in plain
words with no code term is often missed, so read a file end to end once it gives one real suspect.

### The byte-steering keyword

`volatile` written to force a load or a store the original's scheduler happened to emit. It
misstates the code, because the value isn't volatile. Remove it and accept the score. A value really
shared with another thread, the hardware or a re-entered callback keeps it, with the reason on
the line or the line above; `tools/check_volatile.py` fails on one without.

### The assembly body

An `__asm` block or `__declspec(naked)` function that reproduces the bytes of compiled code. It
becomes C. A routine the original wrote in assembly stays assembly, listed in
`symbols/original_asm.csv` and annotated as
[ANNOTATIONS.md](ANNOTATIONS.md#hand-written-assembly) says. `tools/check_asm_bodies.py` fails on any
other.

### The copied library

A library routine rewritten in the tree to score it. The original linked the C runtime and the IJG
JPEG library; so does this build, and `// LIBRARY:` markers pair their routines. The runtime's
statics stay unpaired, and `symbols/retired.csv` marks them `library-internal`. The JPEG library
is built from `third_party/jpeg-6a` with `/Zi`, so its statics pair by name too.

## Rules

[CONTRIBUTING.md](../CONTRIBUTING.md) holds the full rule set. Two more apply to cleanups:

- A cleanup that costs bytes usually removed a second name, not the spelling it replaced.
  Bisect the commit before you record a cost.
- Move one file per measurement. A batch that loses rows cannot be attributed.

## What is left

`python tools/status.py` prints the full board. Every bold figure below must appear in the
output of the tool its row names; `python tools/check_doc_figures.py` checks that.

| Shape | Left | Tool |
|---|---|---|
| Puns kept for an integer store | **0 and 0** | `python tools/pointer_puns.py`: a float or a cell reached through the pun, and a float or a field the cast reaches past |
| Untyped members | **14, 0 open** | `python tools/untyped_members.py --list` |
| Untyped globals | **64, 0 open** | `python tools/untyped_globals.py --list` |
| Classes with bodies in more than one file | **0 open** | `python tools/class_spread.py` |
| Classes in a header of their own name | **272 and 27** | `python tools/header_homes.py`: with marked bodies, and without them but with a virtual function or three member functions; none in a family header or a shared one |
| Int or void* parameters cast to a pointer | **6, 0 open** | `python tools/int_pointer_params.py`: 3 fixed by a callback or table signature, 3 by a virtual slot its family types differently. `AgendaItem::Tick` takes the `const Vec3*` its five callers pass, and `CPlayer::SendPacket` and `SendRawMsg` take a `NetMessage`, the kind byte every sent record and sync block derives from |
| Fields named for their offset | **0, 0 open** | `python tools/placeholder_fields.py --list` |
| Parameter aliases | **0 open** | `python tools/param_aliases.py` |
| Member names with two types | **20, 0 open** | `python tools/check_member_name_collisions.py --types`: open is a builtin pointer beside a class pointer; the rest are separate records |
| Record pairs declaring the same offsets | **2** | `python tools/layout_twins.py`: both chance agreements |
| Records the tree declares as scattered globals | **0 overlap, 0 bound, 0 reads past, 0 short, 0 unsized** | `python tools/scattered_records.py`: needs the original image and the map; the 33 escapes are API out-parameters |
| Vtable slots that run a different body from the original's | **7** | `python tools/vtable_behaviour.py`: folds excluded; six are the concrete `Destroy` and `Insert` in the three pool owner bases, whose original tables are pure, and one is `VisualEffectFade::Start`, which the original folds onto its twin `PaletteScalarFade::Start`; both read the start colour in place, which the client does not |

These shapes are gone, and each tool reads zero:

- Puns that land on a pointer or an integer: `python tools/pointer_puns.py`
- Offset reads: `python tools/offset_reads.py`
- Leading undecoded byte runs: `python tools/undecoded_views.py`
- Views: `python tools/view_classes.py`
- Bodies at global scope, besides `WinMain` and `_purecall`, which the CRT and the linker find by name, and classes and namespaces spread over more than one `.cpp` or in none named for them: `python tools/namespace_spread.py`
- Files with bodies of more than one class: `python tools/class_files.py`
- Globals named for their address: `python tools/placeholder_globals.py --list`
- Functions named for an address, a slot or a resource: `python tools/placeholder_functions.py --list`
- Locals initialised once and not const: `python tools/const_locals.py`
- Headers that do not compile alone: `python tools/check_headers_standalone.py`
- Header guard faults: `python tools/check_include_hygiene.py`
- Forward declarations of nothing: `python tools/check_ghost_forwards.py`
- Retired rows with a stale score: `python tools/check_ledger_figures.py`
- Checkers that read nothing: `python tools/check_battery_reach.py`
- `volatile` with no shared or hardware reason: `python tools/check_volatile.py`
- Extern arrays with no bound: `python tools/check_extern_bounds.py`

A clean build carries 19 warnings: 18 C4244 and one C4700. The compiler warnings entry under
[Kept exceptions](#kept-exceptions) says why each stays.

## The fix list

`goto` and `do { } while (0)` stay, because they are how code of this age was written. Every
other shape below gets fixed, even where the fix costs matching bytes. Record each cost in the
commit body, and in `symbols/retired.csv` when a row moves more than a point.

| Shape | Count | Rule |
|---|---|---|
| Gap and pad arrays | 0 open | A gap nothing in the original reads is a field of its width named `unused` (`m_nUnused`, `m_wUnused`, `m_cUnused`, `m_anUnused`, no `m_` in a plain struct), and SDK records keep their `Reserved` names |
| Fields named for their offset | 0 | A field only a constructor or a bind writes and nothing reads is named `unused` for its width, with letters for several in one class and one sequence across a hierarchy |
| `void*` members | 3 | A `void*` member takes the type of what it holds; the three left are blocks nothing allocates or reads |
| Unions | 0 open, 15 kept | A union stays only as a typed variant that a tag or a resolve step selects and the client reads one way or the other; a packed word otherwise takes getters that shift it |
| Casts of `this` to another type | 0 open, 5 kept | A cast of `this` stays only where the original passes the object as an unrelated type or an external format requires it |
| Puns | 0 open, 1 kept | A float's bits are read only through `FloatBits`, and no typed spelling reads a vtable pointer |
| Offset reads | 0 | A read through a typed base names the field its class declares |
| Widget indices written as numbers | 0 | Widget indices, selection modes and widget ids take the enum in each screen's header, decoded from its build order |
| Hand-expanded helpers | 0 open, 2 kept | A raw read of a field whose class has an accessor goes through the accessor, with a `const` reference return where a value return moves bytes |
| Pointer casts between unrelated types | 0 open | A pointer reaches another type through a typed overlay or union, never through `char*` |
| Strings read across a record's fields | 0 | A player's name is a `PlayerSlotName`, read through `szName` or `cSlotFlags`, and only `PlayerSlotName::GetWireName` returns it as one string |
| Raw buffers | 0 open, 5 kept in net | Network records, sync blocks and audio formats are typed records; a byte buffer stays only for a byte the original never types or an external format |
| Ids written as numbers | 2 | An id takes its name from the original's own lists (SNDFILES.LST, the string table, ENUMIDS.H, HITANIMS.LST) or from the SDK |
| Numbers that carry meaning | 1 | A number takes ENUMIDS.H's name, the SDK's, or a length or sentinel, and a packed word is a typed record, because the client's packing refutes bit-fields (MATCHING.md, Packed words and bit-fields) |
| Frame records | 0; none kept | A local struct that exists only to order a frame becomes named locals, or a `Vec3` where the values are one |
| Found-flags | 0 open; 1 kept by the client | A found-flag becomes a `break`, an early return or an inline helper's return, which VC6 threads into the block order as the client does |
| Names with a number or a letter for a meaning | 0 open; 3 kept from ENUMIDS.H | A name says what the thing holds or does, never its position, a letter or a number, unless ENUMIDS.H authored it |
| Names for a shape | 1 open | A shared body is named for the caller or slot its address belongs to, and a wrapper, scratch value or parameter for what it holds; symmetric operands such as `Vec3::Dot`'s `pA` and `pB` stay |
| One local for two values | 0 open, 3 kept by the client | Each value gets its own local, scoped so VC6 overlays them as the client does, unless the client proves one slot |
| Steering syntax | 0 open, 95 kept | Parentheses round one operand, casts to the operand's own type, bare scopes and renamed single-use copies go; a copy or scope stays only where the client's frame or load order proves it, and parentheses round a product or quotient term stay |
| Inlining pragmas | 0 open; 16 kept by the client, 2 kept by the compiler | A helper the client inlines is marked `inline`, since the game builds at `/Ox /Gf` (`/Ob1`), and `__forceinline` and `optimize` pragmas stay only where the bytes need them |
| Casts that change nothing | 0 open | A cast to the operand's own type or constness goes; `tools/check_self_casts.py` sees only casts of `this`, so read for the rest |
| Integer returns that carry a pointer | 0 open | A function that returns a pointer declares the pointer type |
| Function tables called through a cast | 0 open, 2 kept | Each table takes the signature its callers use, and each entry names its own body |
| Object phases written as numbers | 0 open | `EngineObject::m_nPhase` and `GetPhase` take `ObjectPhase` from `engine/objectphase.h` |
| Plan members switched on as numbers | 0 open | A switch on an opcode's member takes `BehaviourMember`, `PlanChangeMember` or `ActionMember` from `planvm/plannode.h`, each named for its authored keyword |
| Opcodes written as numbers | 0 open | A whole opcode is its `OpcodeCategory` and member from `planvm/plannode.h`, such as `CATEGORY_DEBUG \| MEMBER_SET_TRACE`, and markers and argument tags take their `PLAN_` names there |
| Raw buffers in `graphics/` | 0 open, 6 kept, each for an external type or format | Arrays used as records, walked pointers, copied bytes and view casts become typed records; a byte buffer stays only for an external type or format |
| Raw buffers in `planvm/`, `ui/`, `engine/` and `anim/` | 0 open, 4 kept: the payload, `VerQueryValueA` and the pools for an external type, format or compiler limit, the sentinel the original's shape | Streams, arrays and untyped pointers become typed records; a byte buffer stays only for an external type, format or compiler limit |
| Assembly | 0 open, 9 kept | Assembly stays only for the routines the original wrote in it, listed in `symbols/original_asm.csv` and checked by `tools/check_asm_bodies.py` |
| Manual vtable calls | 0 | No call reads a vtable pointer or indexes a slot by hand |
| Globals typed smaller than their data | 0 open | A GLOBAL's declared size covers the bytes up to the next labelled datum in the original, as its code reads them |
| Compiler warnings | 0 open; 19 kept by the client | A warning is fixed at its cause, with the sign or width the client's compare uses, unless the client's bytes need the code that raises it |
| Uninitialised reads | 0 open; 4 kept by the client | An uninitialised read stays only where the original reads the same unset value |
| Conditional operators | 0 open, 2 kept | A local `?:` macro gives way to windef's `min` and `max`, whose double evaluation the client shows |
| Misnamed functions and false notes | 0 open | A function, field or global is named for what the original does with it, and a note sits above the body it describes and says something true |
| Pointer walks over counted loops | 0 open; 1 kept by the client | A counted loop indexes its array |
| Local CRT declarations | 0 open, 1 kept | A CRT routine comes from VC6's own header |

### Kept exceptions

`void*` members:

- `CollisionShape::m_pUnusedA` and `m_pUnusedB`, `AnimDB::m_pOwnedBlock`: the destructor frees each and nothing allocates or reads it, so nothing names its type.

Unions, each selected by the tag in brackets:

- `DibHeader` (the leading size word, a Windows or an OS/2 header, which `GDI::ReadDIB` compares with 0x28 and 0xc), `SyncBlockSlot` (the slot index), `MovePacket` (`nKind`), the `anim/animclip.h` union (the format), the `common/animnamedref.h` and `graphics/meshmaterial.h` textures (name or index until resolved), `graphics/texcacheentry.h` (`m_bIsD3D`, tested by `TexCacheEntry::DestroyTexObj`), `net/netrecords.h` (the static record), `net/netreceivebuffer.h` (the sender, then the message type or kind), `net/netsendbuffer.h` (the message kind), `PlanWord` (the opcode), `PlanOpcodeSlot` (the opcode's category and member).
- `FloatWord` in `anim/anim.cpp`: `Anim::EulerToMatrix` stores a matrix cell as a float and reads it back as bits and a top half (0x00416c73 to 0x00416cc4).
- `MatrixRow` in `math/transformobject.h`: a row read by column or assigned an axis.
- `ClipVertex` in `graphics/clippoly.h`: `SWRend::ClipPolyAgainstPlane` reads the plane axis by index (0x0047fb9a) and walks the nine components under the mask (from 0x0047fc1c).

Casts of `this`:

- `GameBehaviorObject::Init`'s DO_NET_ACTION and DO_LOCAL_ACTION pass `(Interactor*)this` to `g_pfnPlanActionHandlers`: the client pushes the behaviour itself (`PUSH EDI` at 0x0041c197) and a null pushes 0 (0x0041c1ac).
- `AgendaItem::Activate`'s `static_cast<CAvoidObjects*>`: the original's base body tests `m_wKind` against `Agenda_CAvoidObjects` and reads the swerve and draw targets at 0x28 and 0x2c, and `CAvoidObjects`'s table holds that body.
- `PlayerSlotName::GetWireName`: the game's messages send and print the record as one string, role byte first.
- `StringHeader::Data`: returns `(char*)(this + 1)`, where MFC's string header puts the characters.

Puns:

- `FloatBits` in `math/floatbits.h`: each client reads the float's bits with an integer instruction, such as `CMP dword ptr [ESI + 0x200],0x3f800000` in `ObjectStats` (0x0047497e) and `CMP ... 0xbf800000` in `ModifyOpposTeamFriction` (0x004648bc).

Hand-expanded helpers:

- `MatchStats::AccumulateStat`'s four shooting percentages: the client merges the four tails and keeps the goal count on the x87 stack, so `Percent` costs 13.53 points.
- The null-checked bubble read in `planvm/` stays written out at 34 sites; `GameEngine::GetColObjPtr` serves the rest of the tree.

Pointer casts between unrelated types:

- `AnimClip::Load` copies its single translation from the file buffer with `memcpy`, like its three array copies of the same floats.

Raw buffers in `net/`:

- `GetSendPayloadPtr`'s six-byte offset: the original computes and stores a byte pointer that nothing dereferences.
- The receive loop's one read of the system type under the lock: the client loads the dword once and switches on its low byte.
- The carrier byte `BroadcastState` appends and the sequence byte `RecvMovementPacket` reads: each follows a message of varying length, so the buffers keep `aBytes`.
- The stationary stream's byte cursor and its `StationaryEvent` overlay: an event is five bytes or nine and follows a nibble bitmap sized by the stationary count.
- `NetSessionEntry`: the asserted mirror of DirectPlay's `DPSESSIONDESC`.

Ids written as numbers, left:

- `MainWindow::OnCommand`'s commands 0x7a and 0x9f: no menu, accelerator or sent `WM_COMMAND` in the image holds either, and the numbers are also dialog 122 and `STRING_EXIT_MATCH`, so nothing names the commands. 0x7a reads as a Birds Eye View that posts eye track 0.

Numbers that carry meaning:

- Left: `SyncFileStamp`'s time halves, where a setter or getter costs 12 to 35 points; the WM_SIZE pair, which the client adds unmasked; and colours, palette versions and pixel offsets, which no SDK or table names.
- `CD3DTexture::GetLog2Size`'s 1012: the original's literal where 1024 belongs (0x00435644).
- 0x20000000 stays inside `ACTION_BINDING_HELD`: no reader tests it alone, and the one row that sets it also sets the quick flag.
- Three pronoun tests stay by hand: the client tests a short and indexes another value (0x00405195, 0x004059b6, 0x004630fe).

Found-flags:

- `EngineObject::BuildShadowComponentLinks`: the client clears, sets and tests the flag in `cl` (0x004715b5, 0x004715da, 0x004715ec) and keeps scanning after a hit.

Names with a number or a letter, each authored in ENUMIDS.H:

- `PlanCond::AnalyzePadA` and `B` (`id_DCF_AnalyzePadA`, `B`).
- `PlanFocusArg::GetViewOnrsOppn1` and `2` with their `COB` pair (`id_PRN_ViewOnrsOppn1`, `2`, `1COB`, `2COB`).
- `PlanCond::BungeeVictimLorR` (`id_DCF_BungeeVictimLorR`).

Names for a shape, left:

- The six arguments `CollisionPlane::ForwardTransformRequest` hands a getter that reads none: no call through slot 0x34 pushes its 24 bytes, and nothing calls 0x00405880 directly.

One local for two values:

- `PlanAction::ClothesLineRecoil`'s and `ReflectOffModel`'s `vTemp`, the scaled velocity and then the cross product: each client stores the cross product's unread x and y into the slot `Vec3::ScaleTo` wrote (`ClothesLineRecoil` at 0x00406118 and 0x0040612b over ESP+0x20 from 0x0040603e; `ReflectOffModel` at 0x00405d25 and 0x00405d39 over the slot 0x00405c98 passes).
- `Radar::DrawObject`'s `hdc`, the carrier-arrow flag until `GetDC` fills it: the client stores 1 (0x0043f9ea), compares the slot with 1 (0x0043fb3d) and passes its address to `GetDC` (0x0043fb55).

Steering syntax, among the 88 copies and 7 bare scopes:

- `flTransX` in the three `CollisionCheck` dromer handlers: held on the x87 stack across the centre stores and popped with `FSTP ST0` (0x004523ee, 0x00469998, 0x0046a178).
- `CameraObject::DrawShadowAndTransparentPass`'s `vDir`: y and z load (0x00422c40, 0x00422c43) before the store into `g_lightDirs[0]` that may alias them (0x00422c46).
- The teammate in `PlanCond::RecvrInRangeNView` and `ShootSituation`: loaded before the test whose arm alone reads it (0x00466d61, 0x004687f8).
- The viewport id in `D3DRend::ExecuteScene` and the target in `DDraw::PaintSurface`: loaded before the test whose arm alone uses it (0x004327c5, 0x0043d427).
- The frame top in `InfoBar::DrawPeriodLight`: loaded before the lamp clamp's branches (0x0044fc85).
- The two meter sizes in `InfoBar::InfoBar`: the client stores the unused y (0x004517b8, 0x004519e4).
- Bare scopes whose slots the client overlaps: `InfoBar::InfoBar`'s radar block, whose `SurfacePoint` takes the meter sizes' slot 0x18; `Calibrate`, `CopyBitmap` and `LoadCustomPalette`, which grow their frames without the block; `ReflectOffModel`'s velocity copy and position scratch (0xb0 against 0xbc) and `ClothesLineRecoil`'s position scratch pair (0x34 against 0x4c).

Inlining pragmas:

- `__forceinline` on the `BehaviorOwner` and `AgendaItemOwner` constructors: VC6 runs out of inline budget part way through their callers.
- The 16 `optimize("", off)` pairs: `tools/find_unoptimised.py` lists their 25 bodies as unoptimised in the client, each opening `push ebp`, `mov ebp, esp`.

Function tables called through a cast:

- `CPlayer::NetworkReceiveThreadProc`: a `__cdecl` body the original passes to `CreateThread` as its `LPTHREAD_START_ROUTINE` (0x0042e120 loads and jumps to `NetworkReceiveLoop`, no `ret 4`).
- The bring-up's `GetProcAddress` results.

Raw buffers in `graphics/`:

- The execute buffers, written through the SDK's `d3dmacs.h` macros, which take a byte pointer.
- Packed DIBs, whose colour table, masks and pixels sit where the header's size and counts put them.
- The RLE sprite and MDM1 LZSS streams, which are bytes.
- Surface rows stepped by a byte pitch.
- The model records' self-relative offsets, read through `RecordOffset<T>`.
- `ModelComponent::Read`'s size-prefixed blob.

Raw buffers in `planvm/`, `ui/`, `engine/` and `anim/`:

- `AnimClip::Load`'s payload: a float stream whose Euler triples follow the header's axis order.
- `VerQueryValueA`'s `void*` result.
- The `AttachPak` sentinel `EXTRA_USE_DETAIL`, the original's shape.
- The pools' `void*` `Alloc`: a `PooledOwnerRoot` slot the three owner hierarchies share, and VC6 has no covariant returns (C2555).

Assembly, each idiom read in the original:

- `REP STOSW` in both `Mem` fills and a dead `MOV EAX,[EDI]` in the unrolled one; `SHL EAX,0x11` and `JC` in `Anim::UnpackFixed16ToFloat`; `PUSHAD` in `MDM1::LZSSDecompress`; `OR EAX,EAX` flag returns in `MDM1::RefillInputWindow`; `PUSH ESP` and `PUSHF` in `SWRend::ScanEdge`; `MOV EBX,0x0` and a pushed EBP in `FillSpans` and `DrawLine`; `REP MOVSW` in `BlitRLESprite`.

Globals typed smaller than their data:

- `g_anSubstitutePairs` stays eight ints: the client doubles the kind before it indexes.

Compiler warnings:

- 18 C4244 in `CameraProjectionState::RotateX`, `RotateY` and `RotateZ`, which store a `double` old element into a float: the client holds it on the x87 stack in the order VC6 gives a `double`, and a `float` local or a `(float)` cast reorders the loads.
- One C4700 in `Object::ComputeSteeringDirection`, which returns an unset `vResult` for a kind it does not handle, as the original does (0x00404c99 jumps to the copy-out with that slot never written).

Uninitialised reads:

- `AnimNode::m_nLoopCount`: `AnimScriptTable::ParseSegment` scans `%hd` into its low half (0x004188d9, 0x004188f4), `operator new` does not clear the node (0x0049f2bb), the constructor at 0x00416f30 leaves it unset, and `AnimPlayer` reads the whole `int` (0x004245db, 0x0042525d, 0x00425311, 0x004253c5).
- `PlanCond::AmIKnockedLoose`: without a carrier the client compares the low half of the `pResult` slot (0x00464fa6).
- `PlanCond::AttachedToSender`: the holder slot on the path that never wrote it.
- `Object::ComputeSteeringDirection`: see compiler warnings.

Conditional operators:

- The compile-time array-size tests in `common/staticassert.h` and `common/fieldat.h`.

Pointer walks over counted loops:

- `KeyConfigScreen::OnKeyMessage`'s key search: the client compares the end pointer after the loop, and an index costs 22 points.

Local CRT declarations:

- `_purecall` in `ui/widget.cpp`, which no VC6 header declares.
