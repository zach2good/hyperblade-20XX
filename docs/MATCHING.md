# Matching VC6 SP3

This is the guide to writing C++ that Visual C++ 6.0 SP3 compiles to the same bytes as
`HYPERX.EXE`. Read the first three sections through. Use the rest as a reference: find the
heading that names the symptom in your diff, and try its levers in order.

- A _lever_ is a fact about VC6 that a spelling of the source can use to reach the original's
  bytes. It stays true when the tree changes. The list of rows that still need one does not, so
  scan for a shape before you spend time on it.
- A _wall_ is a residue that no known spelling reaches. [Known walls](#known-walls) lists them.
- The _client_ is the original's side of a diff, and _this build_ is ours. The
  [glossary](README.md#glossary) defines the other terms: row, residue, exact, effective, probe.

The loop for one change is: probe a spelling with `tools/probe_asm.py`, which compiles one file
in seconds, then build, rescore and compare every row against a saved baseline.
[CONTRIBUTING.md](../CONTRIBUTING.md#workflow-for-one-change) gives the commands.
[ANNOTATIONS.md](ANNOTATIONS.md) gives the marker rules and says where a measured refusal goes.

Every lever here is plain C. The project does not take `volatile`, inline assembly or copied
library code to reach bytes; [CONTRIBUTING.md](../CONTRIBUTING.md#rules-for-source) has the
rules. Where no plain lever is known, the row is a wall for now. The one exception is a routine
the original wrote in assembly: see [Hand-written assembly](ANNOTATIONS.md#hand-written-assembly).

## Contents

- [The compiler and the build](#the-compiler-and-the-build)
- [How a function is scored](#how-a-function-is-scored)
- [Reading a residue](#reading-a-residue)
- [Register allocation](#register-allocation)
  - [How VC6 picks callee-saved registers](#how-vc6-picks-callee-saved-registers)
  - [The weight of a reference](#the-weight-of-a-reference)
  - [Scratch registers](#scratch-registers)
  - [Live ranges](#live-ranges)
  - [Return values and widths](#return-values-and-widths)
  - [Commoned constants](#commoned-constants)
  - [A reload the client keeps](#a-reload-the-client-keeps)
  - [Header-sensitive rows](#header-sensitive-rows)
- [Block order and branches](#block-order-and-branches)
  - [A test the client repeats](#a-test-the-client-repeats)
  - [A select with no branch](#a-select-with-no-branch)
  - [Test polarity and placement](#test-polarity-and-placement)
  - [Early exits and flags](#early-exits-and-flags)
  - [Loops](#loops)
  - [Tails, merges and cross-jumps](#tails-merges-and-cross-jumps)
  - [Statement order](#statement-order)
- [x87 and float loads](#x87-and-float-loads)
  - [The default leaf order](#the-default-leaf-order)
  - [Getters round one operand](#getters-round-one-operand)
  - [Other ways out of the sort](#other-ways-out-of-the-sort)
  - [Literals and named constants](#literals-and-named-constants)
  - [Copies, spills and stores](#copies-spills-and-stores)
  - [Comparisons](#comparisons)
  - [Where a value is rounded](#where-a-value-is-rounded)
  - [Arguments and conversions](#arguments-and-conversions)
- [Frame layout](#frame-layout)
  - [Frame size](#frame-size)
  - [Slot reuse and scope](#slot-reuse-and-scope)
  - [Slot order at /O2](#slot-order-at-o2)
  - [The /Od frame](#the-od-frame)
- [Inlining and pragmas](#inlining-and-pragmas)
  - [The inline budget](#the-inline-budget)
  - [Optimisation regions and moved bodies](#optimisation-regions-and-moved-bodies)
  - [Other pragmas](#other-pragmas)
  - [Library inlines](#library-inlines)
- [Switch tables](#switch-tables)
- [Casts, widths and signedness](#casts-widths-and-signedness)
  - [Signedness](#signedness)
  - [Narrow types](#narrow-types)
  - [Masks and shifts](#masks-and-shifts)
- [Packed words and bit-fields](#packed-words-and-bit-fields)
- [Addressing and `lea`](#addressing-and-lea)
  - [Mid-struct bases](#mid-struct-bases)
  - [The rebase](#the-rebase)
- [Constructors, destructors, EH and trylevels](#constructors-destructors-eh-and-trylevels)
  - [Construction order](#construction-order)
  - [Unwind states](#unwind-states)
  - [Destructors and deleters](#destructors-and-deleters)
- [Data, globals and link order](#data-globals-and-link-order)
  - [Placing data](#placing-data)
  - [Link order](#link-order)
  - [Unit boundaries](#unit-boundaries)
  - [Declarations and markers](#declarations-and-markers)
- [Calling conventions](#calling-conventions)
  - [`__thiscall` and members](#__thiscall-and-members)
  - [`__fastcall`](#__fastcall)
  - [`__stdcall`, `__cdecl` and COM](#__stdcall-__cdecl-and-com)
- [Types and records](#types-and-records)
  - [Unions](#unions)
  - [Declarations as inputs](#declarations-as-inputs)
  - [Vtables](#vtables)
- [Known walls](#known-walls)
  - [Link folding](#link-folding)
  - [Why the game builds without `/Gy`](#why-the-game-builds-without-gy)
  - [Other walls](#other-walls)
  - [Screens that look like walls and are not](#screens-that-look-like-walls-and-are-not)

## The compiler and the build

The toolchain is VC6 SP3: `CL.EXE` 12.00.8168 with the SP3 back end `C2.DLL` 12.00.8447, and
`LINK.EXE` 6.00.8447. `cmake/toolchain-vc6sp3.cmake` sets the exact flag strings, and
`CMakeLists.txt` forces them after `project()` so CMake adds nothing:

```txt
cl   /nologo /MT /GX /GR- /Zp8 /Zi /W3 /DWIN32 /D_WINDOWS /DNDEBUG
link /SUBSYSTEM:WINDOWS,4.0 /FIXED /BASE:0x400000 /FILEALIGN:4096 /INCREMENTAL:NO /OPT:REF /machine:X86
```

`CMakeLists.txt` adds the optimisation per source, so no flag overrides another:

- The game's sources get `/Ox /Gf`, which is VC6's `/O2` without `/Gy`. The code is the same as
  `/O2`'s, including `/Ob1`: VC6 inlines only a function marked `inline` or defined in its class
  body. Without `/Gy` only inline and compiler-made functions are COMDATs, so the link folds only
  those, as the original's does. [Known walls](#known-walls) has the evidence.
- The `jpeg6a_objects` target, which holds the JPEG library and `bmpwriter.cpp`, builds at
  `/O1 /Ob2`, as the original's did. The tell is `pop ecx` cleanup and copies as calls.
- `/GX` turns on C++ exception handling, which gives every function with a live destructor an
  unwind state.

A new file joins its family's flag group. An `/Od` unit needs `/GF`, or every literal is its own
writable object and no string pairs. A unit that mixes optimised and unoptimised bodies wraps
each unoptimised body in `#pragma optimize("", off)`, as `bubblecmd.cpp` does for `BubbleCmd`'s
constructor and destructor.

The same objects link twice. `hyperx` links `/OPT:REF` like the original, and in VC6 that also
folds identical functions. `hyperx_sym`, which reccmp reads, links `/DEBUG /OPT:NOREF` and keeps
every body apart, so an operand that names a folded body can only be judged in `HYPERX.EXE`.

## How a function is scored

reccmp pairs each `// FUNCTION: HYPERX 0xADDR` marker with the symbol under it in
`HYPERX_sym.pdb`, disassembles both, and aligns the two listings. `reccmp-project.yml` names the
target, `HYPERX`. `symbols/float_constants.csv` names the x87 constants, because the original is
linked `/FIXED` and has no relocations to find them by.

```sh
python -m reccmp.tools.asmcmp --target HYPERX --json build/report.json --silent
python -m reccmp.tools.asmcmp --target HYPERX --verbose 0xADDR --no-color
```

The first writes `build/report.json`, which holds every row's score and its whole instruction
diff. The second prints one function's diff. `cmake --build build --target match` runs the
scorer over the whole tree.

- A row is exact or it is not. Closing a function is worth its whole size, so rank work by size
  against residue (`python tools/near_misses.py`), not by the percentage.
- An effective match reads 100.00 but differs in something reccmp judges harmless, such as a
  register name. It counts in the accuracy mean and adds nothing to the exact byte share.
  `python tools/progress.py` prints both.
- reccmp names a symbol operand by the marker's name on the original side and the PDB name on
  ours, and compares the two as text. For the exceptions, see
  [Data, globals and link order](#data-globals-and-link-order).
- reccmp cuts the original's listing at the length of ours. A jump table or an epilogue past
  that point is missing from the diff, not from the function.
- A diff that shows only the differing lines hides the lines that pair. Read a hunk with its
  paired lines before you call something a decode fault.

## Reading a residue

Classify the residue before you pick a lever. These tools read `build/report.json` and need no
build. Run each with `python`:

| Tool | What it tells you |
|---|---|
| `tools/residue_kinds.py 0xADDR` | how much residue is register naming, how much is structure |
| `tools/fault_classes.py` | which rows are folding, data address or x87 walls |
| `tools/slot_residue.py` | which rows have frame-slot faults, and of which kind |
| `tools/missing_this.py` | rows where the client loads `ecx` for a call and ours does not |
| `tools/global_drift.py --indexed` | globals reached by a scaled index at the wrong address |
| `tools/prior_work.py 0xADDR` | every recorded spelling for an address |

`prior_work.py` prints the source file's own header among its hits; read that first.

- Where the residue is the shape of an expression, source reaches it. Where it is one
  instruction scheduled differently inside one statement's expansion, or one value in another
  register with the same code, it usually does not.
- A drift in displacements that recovers bit by bit is register allocation. A matched `+N` then
  `-N` is reccmp's aligner. A step that does not come back before the epilogue is a real
  difference in the code.
- An `[esp+N]` difference is not a slot difference until you subtract the stack depth, since VC6
  pushes `ebx` and `ebp` mid-function too. Convert every `[esp+N]` to a frame offset, per block,
  before you call two slots swapped; depth accumulated across a branch's arms invents a frame.
- About two in five residue lines are a `mov`, which is register and slot naming. A row whose
  residue is mostly `mov` has little for source to reach. Look for the lines that are not.
- Residue counts are only comparable within one alignment. The score decides.
- A shape repeated across several rows usually has one cause. Sweep every non-exact diff in
  `report.json` for the shape, then find the source cause. When the shape repeats at calls to
  one function, the cause is that callee's declaration.
- A lever found by removing something is also a lever when you add it. Record which way each
  direction pushes.
- A lever holds only under the conditions it was measured in, and file position is one of them.
  After a body moves into a larger file, re-measure any lever its note says depends on being
  first or alone in the unit.
- Two recorded failures seldom combine into a success. The change that moves blocks decides the
  alignment and hides the other one. Re-measure a file's recorded failures after a structural
  change that paid.

## Register allocation

Most residue lines in an unfinished row are one value in another register, or one displacement
moved by a push. First find out whether the client holds a different number of values, or the
same values in a different order.

### How VC6 picks callee-saved registers

Only a value that lives across a call qualifies for `ebx`, `ebp`, `esi` or `edi`. A value that
dies before the next call takes a scratch register (`eax`, `ecx`, `edx`), whatever you name it.

The count of values live across a call sets the saves. Measured with the build's flags:

| Values live across a call | Saved |
|---|---|
| 2 | `esi`, `edi` |
| 3 | `ebx`, `esi`, `edi` |
| 4 | `ebx`, `ebp`, `esi`, `edi` |
| 5 | the four, and one value spills to the frame |

So where the client saves `ebp` and this build does not, the client's source keeps a fourth value
live across a call: find it. Every `[esp+N]` below the missing push is four bytes off, so the
residue is mostly displacements. Where this build saves one more register, it keeps a value
alive that the client lets die.

The order among qualifying values is by weighted reference count. VC6 colours greedily: the
value with the highest weight takes `esi`, and each next value takes the first of `esi`, `edi`,
`ebx`, `ebp` that no higher-ranked value overlapping it holds. So two values with disjoint ranges
share a register, and a value that finds all four held spills, even where a lower-ranked value
with a shorter range gets one. On a tie the value defined first wins. Definition order reaches
the allocation, so assigning `i = 0` earlier moves registers; moving a declaration, or the order
of use, does not. VC6 gives `this` a register at three references and spills it with `push ecx`
at two. [The weight of a reference](#the-weight-of-a-reference) says what a reference is worth.

Only references that survive optimisation count: `p = p + 1` is one, a local copy inside a loop
is coalesced, and an indexed loop is strength-reduced onto one walker. A register carries several
ranges, so counting register mentions in a listing doesn't apply the rule.

A push stands where its register's live range starts, so `push ebx` and `push ebp` can appear
mid-function. To move a push, move the range: change where the value is first defined or which
value holds the register.

The count is necessary, not sufficient. Buying the client's fourth push can still permute which
value lands in which register, so a paired prologue is not progress on its own.

### The weight of a reference

`tools/alloc_weight.py` measures this: it finds the count at which one of two contested values
takes `esi`. A reference weighs

```text
weight = W(block) * 2^(loop depth)
W(block) = the number of distinct register values the block references
```

A register value is anything VC6 holds in a register and references twice or more in the
function: a named local or parameter, a call result named in a local, a temporary from
arithmetic such as `c + 1`, or a constant used twice, including the zero that each `if` tests
against. A value pushed straight from memory, an unnamed call result passed straight to a call,
and a global read once between calls are not register values. A value used on both sides of a
branch is loaded above the branch, so it counts in that block too.

In straight-line code every reference has the same `W`, so the rule is a plain count. Across
blocks, the contested values compare as below. Each line is an `alloc_weight.py` template, and
`y/x` is what one `$Y` reference is worth against one `$X` reference. Each template starts
`int x = f(a); int y = f(b);`. The probe counts both definitions, so the ratios are exact
multiples of the measured `n`.

| Template | y/x | Why |
|---|---|---|
| `$X $Y` | 1 | one block |
| `$X if (g) { f(1); } $Y` | 1/2 | entry has `x` and `y`, the join only `y` |
| `$X f(c); f(c); if (g) { f(1); } $Y` | 1/3 | a third value in the entry |
| `$X if (g) { f(1); } $Y f(c); f(c);` | 1 | a second value in the join |
| `$X if (g) { f(1); } $Y int z = f(c); f(z);` | 1 | a named call result is a value |
| `$X if (g) { f(1); } $Y f(f(c));` | 1/2 | an unnamed one is not |
| `$X if (g) { f(1); } $Y if (h) { f(2); }` | 2/3 | two tests make zero a value in both blocks |
| `$X do { $Y } while (--g);` | 2 | a loop doubles |
| `$X do { do { $Y } while (f(1)); } while (f(2));` | 4 | and doubles again |

What this means for a callee-saved swap:

- A near tie moves with values that the two candidates do not share. Naming a call result,
  folding a named result into its call, or a test against zero changes `W` for every reference
  in its block, so it can move registers on lines far from it (`PlanAction::SyncSystem`).
- Count the references block by block. Where the client prefers the value referenced in the
  busier block, the source probably has one more register value in that block.
- Branch probability does not enter. An `if` arm, an `else` arm and the join all weigh by their
  own `W`. A nested `if` costs nothing more.
- A loop reference has no fixed multiplier. A ratio such as twelve to one is this rule seen
  through one block pair. Measure the template before you trust a multiplier.

Aggregates in the frame don't use these weights. See [Slot order at /O2](#slot-order-at-o2).

### Scratch registers

At `/O2` VC6 hands `eax`, `ecx` and `edx` round-robin to the unnamed temporaries of successive
statements. A call does not reset the turn, and a call result does not take a turn. A load that
only feeds a test takes `eax` out of turn. So `f(g1); f(g2); f(g3); f(g4);` loads `eax`, `ecx`,
`edx` and `eax`, and where the client reloads one field into the same register twice, this build
reloads it into the next register.

Count the turns in a repeated block. A block that spends four turns starts one register on each
time it repeats. Where the client restarts at `eax` in every copy, its block spends three or six
turns.

A field read through an inline const getter can restart the turn. In `Screen::ClearBlack` the
client loads `g_gameApp.m_hWnd` into `eax` for `GetDC` and again for `ReleaseDC`. Written as the
field, the second load takes `ecx`; written as `g_gameApp.GetMainWindow()`, both take `eax`. In
probes this needs the field at a non-zero offset and the first call's result kept in a
callee-saved register. A free inline function does not do it. Where every read of one field takes
`eax` in the client, read it through the getter at every site, the first included: with
`ObjectDef::GetModel()` at only one of the three sites in `EngineObject::InitModel`, the next
read takes the next turn.

A load into a named local can take the register its address operand frees. Where the client
loads through a register into that same register (`mov eax, [edi+eax*4]`) and this build takes
the next turn, name the loaded value: `ParseUtil::FindKeyword` has
`const char* const pszEntry = ppszTable[iEntry];` in its loop. It does not always move:
`const int v = p[i]; f(v);` emits the same bytes as `f(p[i]);`.

### Live ranges

A named value's range starts at its enclosing block. To shorten a range, declare the name in the
innermost block that holds its uses; for a read inside a loop, that is the block the loop sits
in. Where VC6 forwards the value, a named local emits the same bytes as the inline expression:
naming changes the range, not the register. Only a use in a later statement keeps a value alive.

Read a global inside the guard that consumes it, not above the guard. This shortens the range,
and can remove a spill.

Arguments follow the client's evaluation point:

- Inline an argument where the client computes it at the call. Named locals for call arguments
  can make an earlier value's last use later, and move it to a callee-saved register:
  `PlanAction::SyncSystem` needs the actor's `GetSlot()` written in `BroadcastEvent`'s argument
  list, where a named `nTeam` gives the actor `esi` and the clock `edi`.
- Name the arguments the client computes before its first push. VC6 pushes an inline argument as
  soon as it computes it, so where the client works out two argument sums into registers and then
  pushes the whole list, the source named the sums in locals above the call, as
  `GameApp::SetupViewports` (0x0044b395 to 0x0044b3ae) names its centring insets. A width local
  read at every use undoes it there, so measure the locals around the named sums too. If no
  declaration order gives the client's order, the locals prove nothing and stay out.

Give each packed word its own local. Where the client loads the second word into the first
word's register, one local is reading two words: name the second. A value the client computes
early in the second word's block is a local assigned at the head of that block. Example:
`MoveCodec::Decode`.

A byte the client loads once into `al` and never spills is a memory read at each use: write
`pPacket[1]` at every use and VC6 commons the loads. An `unsigned char` local spills to a
parameter home instead. Example: `PlayerBase::RecvMovementPacket`.

A byte stored from `al` after a `test al, al`, and read back with `and reg, 0xff`, is an
`unsigned char` local; the callee can still return `int`. Example:
`PlayerBase::NetUpdateMultiObject`.

One load stored into two slots is two locals that each read the source,
`a = p->x; b = p->x;`. VC6 copy-propagates `b = a;` and `b = a = x;`, even through record members,
so the second slot vanishes. Example: `GameEngine::SelectSubstitute`.

A pointer parameter the client loads into one register (`mov esi, [param]` / `mov ebp, esi`)
and walks is the walker. Advance the parameter itself and keep the start in a local copied from
it. Example: `ParseUtil::ReadLine`.

A copy the client tests (`mov eax, edx` / `test eax, eax`) is a second read of the member:
`pWalk = m_p;`, compared against `m_p` in the loop. Example: `Spline::~Spline`.

Re-read a field where the client re-reads it. VC6 forwards a stored or loaded member through
later statements, so if the client loads a field again before each call, don't hold it in a
local. Where the client keeps a field in a callee-saved register across a call, name it:
`strcpy(new char[strlen(p->s) + 1], p->s)` reloads `p->s` after `operator new`, and
`const char* const pszName = p->s;` above it keeps it in `esi`.

VC6 does not forward a store through an array member into a read through a pointer to it. Where
the client stores a constant and reads it straight back, `m_apSlots[0] = 0;` then
`p = *ppSlot;` keeps both, while a store through `ppSlot` folds the read and the loop behind it.

Put the first call inside the second call's argument list when the client keeps a vtable pointer
in `ebx` across both. A member call loads its object's vtable before it evaluates its arguments,
so the vtable pointer then lives across the inner call.

A vtable load between two arguments' call chains is an argument assigned in the statement
before: `pCurrent = pVm->GetActor()->GetPlanState()->m_pCurrent;` then
`pInteractor->Bind(pVm->GetActor(), pCurrent);`. Written in the argument list, the vtable load
comes first. Example: `PlanCond::EngageWhichOpponent`.

Statement order decides which value owns the register an input arrived in. VC6 computes the
first-stated value through a copy of the register and the second in place. It also gives `eax`
to the value the first statement defines and reuses `eax` once that value is stored. The same
definition-order rule reaches constant substitution: define a loop index after a found flag, and
VC6 can use the register that holds its zero in place of a literal in the rotated guard.

An object fetched before the other operand's call in a compare is a named pointer. Write
`pActor = pVm->GetActor();` and compare against `pActor->GetSlot()`; written inline, VC6 makes
the whole member call first. Example: `PlanCond::IsSenderTeammate`.

VC6 calls the later-declared function first in `a->F() + a->G()`, whatever the source order.
Where the client calls them in the other order, declare the two in the order of their addresses
in the original. Example: `PlanCond::RandomEmptyArmory`.

A zero kept in a register through a test is a variable assigned zero in the `else` arm,
`if (c) { x = f(); } else { x = 0; }`. A default `x = 0;` above the `if` makes the register the
variable, so the next zero test uses an immediate. Example: `GDI::LoadBitmapWithPalette`.

A folded redundant test ends a live range. Deleting a test that looks pointless can move every
register after it, so measure it.

Look for a variable the client already has. A callee-saved swap is often a local the client
names and this build folded, such as a descending cursor named from the raw count
(`lea esi,[ecx+eax*4-4]` then `dec eax`), or an increment placed after a copy,
`nUsed = nIndex; nIndex++;`. A value the client computes early is usually an existing variable
adjusted in place.

A return read from a frame slot is its own local. Where the client copies a working pointer into
a slot once and loads `eax` from it at the single exit, write `pResult = pNode;` once the pointer
is made, `pResult = 0;` on each failure path, then `return pResult;`. Example:
`ModelDatabase::LoadLOD`.

A parameter the client loads below a loop's entry test is a counter copied after an early return:
`if (pAt == 0) return;`, then `nPriority = nFirst;`, and step the copy. A copy made above the
loop loads above the test. Example: `GameBehaviorObject::AssignPriorities`.

A `this` spilled with `push ecx`, where the client keeps it in a register, can be a call result
written in another call's argument list. Name the inner result in its own statement and VC6
counts `this` high enough to keep it. Example: `AnimRegistry::BindAnimSets`.

A sum the client forms with `lea` once and compares in a loop is written in the loop's
condition: `dwStart = timeGetTime(); while (dwStart + 200 < timeGetTime())`. VC6 hoists the
invariant sum into a register of its own. Example: `GameSession::OpenSession`.

A double cleared to zero is two dword stores the scheduler can split. Where the client stores the
high halves after the integer clears, write the double assignments after the integer ones
(`FrameTimer`).

One loop's counter can be the next loop's variable: two loops over one variable remove a whole
`push edi` from `D3D::FindDeviceByFormat`.

Walk the record base, not a field. A walker that points into the middle of a record, or starts
one element in, makes VC6 build a second induction variable.

A mark pointer in a scratch register is not reachable. Check the register class first: if the
swapped registers are `eax` and `ecx`, it is the allocator.

Globals the client reloads after a call on one path only are read directly at each use. VC6
keeps the values in registers and reloads them on the path that called; locals that copy them
shift every register. Example: `GameApp::InstantiateState`.

### Return values and widths

The declared return type decides the registers, even when no caller reads the result. A function
that answers `this` keeps `this` in `ecx` when declared `void`, and every store changes register.
A `void` function whose caller reads `eax` returns a value, `mov eax, ecx` at the end means the
function returns its object, and a callee that ends `mov eax, pOut` returns its out-parameter. A
tail `jmp` needs matching return types. Change a return type in the header too, since it is part
of the mangled name.

Read the return width off the epilogue. `mov al, 1` cannot come from a four-byte answer, so
declare `char`. An `int` return of a byte value forces `movsx`, can hold a register open, and
puts every `[esp+N]` four bytes off. Check the width first on any near miss.

The callers show the width too. A caller's `test al, al` on a call's result means the callee is
`unsigned char` (`GameEngine::IsExhibitionMode`). A caller that masks a result with `& 0xff`
means the callee's return width is wrong. Changing a return width changes every caller, so
compare the whole tree.

Where the callee's own bytes prove an `int` return (`mov eax, 1`) and a caller still tests `al`,
the caller narrowed: write `(char)IsAAPrefix(szToken) != 0`. Example: `PlanVM::LoadSymbolTables`.

Return a comparison through a local. `bReached = (unsigned char)(a == b); return bReached;` gives
the client's `sete al` where the inline form adds a saved register.

Write one `return` at the end where the client moves one saved register into `eax` at every
exit. VC6 forwards the stored member value to the single return.

A struct returned in one register pair from every case of a switch is two scalars, assigned in
each case and copied into the struct once after the switch. Fields assigned in each case take
whatever register each case frees. Example: `ColorPalette::GetFrameRange`.

A float local never takes an integer register; only an `int` local keeps a float's bits in a
callee-saved register across calls.

`mov esi, 1` then `mov eax, esi` before an epilogue is a result variable, and a bare `mov eax, 1`
in one arm is that arm's own `return 1;`. Leave the other arms on `nHandled = 1; break;`; with
every arm on the flag, VC6 propagates the constant into all of them. Example:
`GameSetup::SessionListDialogProc`.

A second `idiv` the client does not have is a return value nobody reads. If no caller reads the
answer, declare the function `void` and drop the `return`: the quotient the client leaves in
`eax` falls out of the remainder's divide. Example: `CameraObject::UpdateDirSmoothing`.

A caller that writes nothing to `al` between a call and its epilogue returns the call's answer.
Write `return F(0);` and give `F` the return type; it is not a function falling off its end.
Example: `AnimPlayer::UpdateTick`.

### Commoned constants

VC6 puts a constant in a register for stores, not for other uses. A zero or a one stored several
times is materialised once in a callee-saved register (`push edi` then `xor edi, edi`). Pushes of
the constant never trigger it, and a constant used once never gets a register.

Where the client stores the literal and this build commons it, try these in order:

- Remove one store of the constant. The rule has a threshold, and one use fewer can free the
  whole register: write a clearing loop as `for (i = 0; i < n; i++) a[i] = 0;`, not a guarded
  do-while that stores at `[i - 1]`.
- Write a zero store as a declaration initialiser, `int nLongest = 0;`, not a later statement.
- Drop a redundant second zero: `for (;;)` where `i` is already initialised.
- Put a byte's nought above the `int` zero it would share, where the client stores
  `mov byte ptr [x], 0` and this build stores `al` (`SoftTex::CompressToRLE`).
- Write short clears as `memset`. VC6's inline `memset` makes its own zero in `eax` and copies
  the destination pointer; assignments share the function's commoned zero. The tells are a
  client-only `mov reg2, reg1` before a run of identical stores, and `lea reg, [this + K]` with
  stores through it. A cleared record is one `memset` too: `SoftTex::SoftTex` clears its
  `SoftTexImage` with `memset(&m_image, 0, sizeof(m_image))`. Zero stores before the `memset`s
  share their zero with them; after them, each `memset` makes its own.
- A stored one that the client keeps in its own slot has no plain lever. `const`, `register` and
  a plain local fold back to the immediate. The row is a wall for now.
- A failure code the client loads into a callee-saved register at the head and returns from it is
  a result variable that one `return` after an `if`/`else` reads. Assign the result in every arm,
  `nResult = nLength;` or `nResult = 0;`, so the constant reaches the return only on the failure
  paths. An early `return nResult;`, or a result no arm reassigns, folds the constant. Example:
  `CPlayer::SendPacket`.

Which constant of a run of stores goes in the hoisted register follows the source order, even
where VC6 emits the stores in address order. Sweep the statement order by script before you call
it a wall. `D3DRend::InitRenderState` needs its `FALSE` store first.

`xor eax, eax` at the head, shared by a compare, a store and a push, and still in `eax` on the
exit that skips the call, is a result local set to zero: `char bResult = 0;`, assigned the
call's answer in the arm, then `return bResult;`. Example: `AnimPlayer::ConditionalDispatch`.

A zero made above a branch that one arm's loop counts in is the counter cleared above the branch:
`nAt = 0;` before the test, `for (; nAt < n; nAt++)` in that arm and `for (nAt = 0; ...)` in the
other. Example: `PaletteScalarFade::ApplyPhase`.

A flag the client keeps in `bl` from an earlier `mov bl, 1`, with no store of one on the set path,
is assigned in both arms of its test. Write `char bFlag;` in the block that tests it, then
`if (c) { ...; bFlag = 1; } else { bFlag = 0; }`. VC6 finds the one already in the register and
drops the store. Initialised at its declaration, the flag lives from the top of the function and
spills. Example: `CTrackPasser_Receiver::Process`.

A constant loaded into a register for one store or one push is a struct copy. VC6 keeps a struct
copy's register shape after it propagates a constant into a member, so `mov ecx, 2` then
`mov [esi+0x28], ecx` is a `Point` assigned whole, and `mov edx, 1` then `push edx` is a `Point`
passed by value. Example: `HUD::PlaceSurfaces`.

A global stored in a loop and used after it with no reload is the loop's variable. Write the
global in the loop and after it; a local mirrored into the global gets its own register. A struct
assigned whole there copies through `eax` and `ecx` in turn and lets a `mov eax, 1` in before the
last pair. Example: `DDraw::EnumDisplayModes`.

### A reload the client keeps

Here the client keeps a load or a store that VC6 would forward, common or sink: it reloads a
value mid-block, loads one address twice, keeps a store after a `call`, or computes a value above
a branch where this build computes it in the one arm that uses it. The original had no qualifier;
its statements were ordered or named differently. Try these in order. Where none moves the row,
it is a wall for now: record the spellings tried in `symbols/retired.csv`.

Name the intermediate pointer and bind through it. A local that holds a member read early makes
VC6 load it early. Name the object instead and read the member at the use:
`pPlan = pSystem->GetPlanState();` then `pInteractor->Bind(pSystem, pPlan->m_pCurrent);`.

Store through the expression the guard just tested, not an earlier local. Where the client
re-reads a field off a pointer it has just tested, write
`g_pGameSession->apPlayers[nSlot]->pState->nSlotFlags = nSlot;`, not `pPlayer->pState->...`.
VC6 commons the tested slot but not the field read off a local, so the earlier register's range
ends (`Network::OnPlayerConnected`). VC6 commons a field read only between two reads spelt through
the same path, so where the client loads one pointer field twice for two arguments, read one
through the local and the other through the table (`Network::OnAcceptPlayerSlot`).

Declare a `const` local at the point the client loads, since VC6 puts the local's `fld` where the
declaration stands ([Other ways out of the sort](#other-ways-out-of-the-sort)). Split the
statement ([Copies, spills and stores](#copies-spills-and-stores)). Read the leaf the client
loads first through a by-value getter ([Getters round one operand](#getters-round-one-operand)).

Move the fold behind an inline helper. VC6 does not fold a repeated test or forward a
just-stored value across an inline boundary, so read a value the client reloads through a small
inline helper such as `Vec3::Magnitude`. VC6 still commons two identical loads through one
helper. See [A test the client repeats](#a-test-the-client-repeats).

Compare against a named `extern const float` where the address is a variable, not a pooled
literal (see [Literals and named constants](#literals-and-named-constants)). With a literal
operand, VC6 moves a nearby store into the gap between `fcom` and `fnstsw`.

Store through an inline helper to keep a store above a later load: a small inline setter such as
`pNode->SetNext(0);` keeps a member store where the statement stands
(`GameBehaviorObject::TickAndSortMessages`). It doesn't work at every site, so measure it.

A store, then a reload through the address it went to, is a chained assignment. In C++ the value
of `a = b` is the object `a`, so `x = y = z;` stores `z` into `y` and reads `y` back for `x`. A
named pointer and two statements give `mov` and `add` in place of the client's `lea`. Example:
`PlanAction::SetLocation`.

Three `fst` of one product ending in `fstp`, and a later read of the first member from memory, is
a chained assignment with no local: `m_c = m_b = m_a = x * 0.5f;`, then read `m_a`. A named
`const float` holds the product on the x87 stack instead. Example:
`CameraProjectionState::Configure`.

Writes through a pointer to a member stop VC6 forwarding them. Where the client reads back a
member it has just stored, store through a pointer: `pDelta = &m_vDelta; pDelta->z = ...;`.
Example: `AnimPlayer::ComputeScale`.

Reads through a pointer keep a later member store below them, because VC6 cannot prove the reads
and the store apart: `pAxis = m_transformC.m_a[2];` then `pAxis[0]`. Example:
`ParticleSystem::Update`.

Member loads the client never lifts above earlier member stores go through an inline getter that
returns a reference. VC6 hoists a read of `m_sFrame.m_ptSize` above stores to other members of
`this`; `const SurfacePoint& GetSize() const` keeps every load below them. For stores the client
does not interleave, set each through an inline `SetPosition(const Point& pt)`. Example:
`InfoBar::LayoutSurfaces`. A by-value getter adds temporaries the client lacks.

A call through a parameter just stored to a global is a call through the global. Where this build
sinks a store below the pushes, read the object from the global the function has just written;
the reread keeps the store in place. Example: `D3D::GetCurrentDisplayMode`.

Where a removed single-use copy fixed only a load's place, read the field through an inline
getter. VC6 treats an inlined getter's result as an expression of its own, so it often loads where
the copy did. A float read through a member pointer takes a by-value getter on the owner,
`GetSpeed() const { return *m_pSpeed; }`: the divisor of `g_flOneAgendaItem / GetSpeed()` loads
first, as in both `IntegrateSpringAndGravity`, and `GetBoost() < a + b` loads the boost before the
sum and compares with `FCOMPP` (`CSpeedBurst::Process`). A getter does not hoist a load above a
branch or ahead of another object's load (`D3DRend::ExecuteScene`).

A shift the client makes with `SHR` on a signed argument goes through an unsigned parameter, as
in `PlanCond::MessageNibble(unsigned int nData, int nShift)`, with no cast written.

Re-measure after an order change. A spelling that compensated for a wrong statement order becomes
a cost once the order is right.

### Header-sensitive rows

A few functions break a tie on their symbol numbers, modulo 64. The front end numbers every
declaration it reads from one counter, the counter that also names the `$L` labels and the
block-scoped locals (`_hBitmap$40433`) in a `/FAs` listing. A `typedef` or an `extern` takes one
number, an enumerator one, an empty struct eight, each data member one more, and an inline member
function about three. Insert N typedefs into a header and probe the unit: for every function in
`netsetup.cpp`, `splashdialog.cpp`, `keyconfigdialog.cpp`, `planvm.cpp` and `gameengine.cpp`,
N = 64 and N = 128 give the code of N = 0. Only the count matters, not names, kinds or which
header holds them. Declarations after the function change nothing.

The difference is code, not addresses: other registers, and whether VC6 commons a subexpression.
reccmp scores it correctly, so it is not a data-address wall. Measured cases:

- `NetSetup::OnPlayerDisconnect`: from its exact count, numbers shifted by 5 to 10 or by 57 to 62
  make it zero `ebx` in place of `ebp`, save one register fewer and not common
  `lea esi, [edi*4+176]`, 59.24. Pooling the 0.1 literal moved it to 59.24 the same way.
- `SplashDialog::PaintSplashBitmap`, exact in the tree, reads 93.94 with `globals.h` grown by 38
  to 45.
- `PlanVM::LoadAnimationLists` moves with the count too (91.09 and 90.91) but is retired as a
  fold.
- `KeyConfigDialog::Init` is not numbering: its two variants (94.38 and 94.69) both fall short.

Shifting all 356 units by 5, 13, 22, 30, 38, 47 and 57 (a forced include of N typedefs) moves
these four functions and no others. A narrow window between those shifts could hide a fifth.

Nothing in the source pins such a row. The original's counter at the function is the sum of
everything its unit declared before it, which only the original's headers would reproduce.
Trimming includes does not help: the four units use every header they include. So:

- A header change that moves only one of these rows is a numbering shift, not a cost of the
  change's own code. Judge the change on its typed merit. The row moves again with the next
  header change.
- To confirm, put N typedefs just before the function's marker and run `tools/probe_asm.py` for
  N from 0 to 63. If one N reproduces the new code on the old header, the move is numbering.
- Never add, keep or delete a declaration to set the count. That is steering.

Any header edit can do this, even an inline member added to `vec3.h` or a pooled global's
declaration removed from `globals.h`. Measure a new getter's header on its own, and a header
change against the whole tree.

## Block order and branches

A row whose residue is a block out of place, a merged tail or a missing epilogue usually has a
source cause.

### A test the client repeats

Two identical tests in a row, on one register, are an inline helper's own test. VC6 folds
`if (p) { if (p) ... }` written in one body into one test, but does not fold a test across an
inline boundary. Put the inner test and its body in a small inline function:

```cpp
static inline void FreeName(char*& pszName)
{
    if (pszName != 0)
    {
        delete[] pszName;
        pszName = 0;
    }
}
```

and call it under the outer test, `if (m_pszName != 0) { FreeName(m_pszName); }`.

A float clamp the client tests twice wants its guard and its arm spelled differently.
`if (MAX < x || MIN <= x) { if (MAX < x) { x = MAX; } } else { x = MIN; }` lets VC6 thread the
guard's true edge past the arm's identical test. Spell the guard's test non-strict, `MAX <= x`:
the arm answers the bound at equality either way, and VC6 keeps both tests, at the price of
`TEST AH,0x41` for `1` on one of them. An inline helper holding the arm's test does not stop the
threading for a float compare. After an `&&` guard that returns, spell the later test non-strict
instead. Example: `PlanAction::ModifyDamage`.

`mov eax, [x]` / `dec eax` / `jne` is a one-case `switch (x) { case 1: ... break; }`.

### A select with no branch

`neg`/`sbb`/`and`, or `setcc`/`dec`/`and`/`add`, choosing between two constants is an inline
helper that returns one of them. The tree writes no conditional expression, and an `if`/`else` in
the body gives two branches or two calls. A small inline function whose body is
`if (c) { return a; } return b;` gives the client's select when it is expanded. The helper is a
static member, since a free function fails `tools/namespace_spread.py`. Example: `HUD::RowColour`
makes `HUD::UpdateFoeScoreboard` exact.

In a loop, the helper's shape decides whether VC6 reduces the subscripts. In
`GameEngine::ReadSaveFile` the two-return helper keeps the seat loop on a scaled index, while
`nSlot = nTeam; if (nSeat != 0) { nSlot += 2; }` gives a branch but moves the loop onto the
client's byte cursor and scores higher. Measure both.

For `mov reg, 0` / `sets` / `dec` / `and`, see
[Casts, widths and signedness](#casts-widths-and-signedness).

### Test polarity and placement

VC6 falls through into the block the source wrote first, the `if` body, and jumps to the `else`.
A `je` against `jne` at the same address means the two bodies are in the other order, so swap
them rather than add a `!`. A jump to the success block means the source tested for failure,
`if (Open() == 0) { return Teardown(); }`. An inverted arm can make a whole function read as
swapped callee-saved registers, so check polarity first.

A choice loaded between the `test` and the jump (`test ah, K` / `mov eax, A` / `je` /
`mov eax, B`) is an `if`/`else` with both assignments in arms, the first on the test that picks
`A`. A default assigned above the `if` lets VC6 put the `mov` in the FCOMP latency slot. Example:
`CameraObject::DrawShadowAndTransparentPass`.

VC6 sinks an `else` below a `then` arm that cannot fall through. Only a compound cold arm sinks:
`if (fail) complain; else rest;` with one comparison stays in line, while
`if (ok) { rest } else { complain; }` sinks. To place a cold block after the success return, nest
the body in the success arm and put the complaint in the `else`.

VC6 flushes a deferred cold block at the end of the guard that owns it. Where the client tests one
flag twice and this build commons the test, split the guard into two `if`s.

VC6 usually places next the block the last test branches forward to. Treat this as the default,
not a law: the client's order is sometimes reachable at a cost elsewhere.

Where two arms of a ladder give the same answer, rewrite the tests so VC6 commons the second test
with the first arm's short-circuit branch. Reordering the blocks with `goto` builds the same image.

A client that loads a stale spilled value on a ladder's fall-through path has no final `else`.
Otherwise repeat the value in a final `else` so VC6 folds the arms.

Compare against the client's constant. `n > 1` and `n >= 2` emit different code, and so do
`n < 1` and `n <= 0`. `xor` then `jz` is `^`. `test` then `jbe` is an unsigned `> 0`. When a name
replaces the literal, keep the client's operator and write the name's neighbour:
`nRow > KEY_BINDING_COUNT - 1` in `KeyConfigScreen::OnKeyMessage`.

Write an answer of one or zero as a flag. `int n = 0; if (a < b) n = 1;` is not the same code as
`n = (a < b);`.

Assign both arms. `nIndex = nSlot; if (c) nIndex += 2;` and
`if (c) nIndex = nSlot + 2; else nIndex = nSlot;` load the slot into different registers.
Example: `PlanCond::PassOrRunByRatings`.

A raw word and the value resolved from it are two variables. Where the pronoun arm's answer and
the raw argument sit in two registers, joined by `jmp` over `mov eax, ecx`, write
`nRaw = pArgs[1]; if (pronoun) nTarget = f(...); else nTarget = nRaw;`. One variable reassigned
in place loads the word into the answer's register. Example: `PlanAction::SendMsg`.

`cmp ax, K`, then `movsx`, then the branch is a `short` tested and an `int` assigned in both
arms, `if (nKind < 4) nOther = nKind + 2; else nOther = nKind;`. A cast on an `int` widens before
the compare. Example: `PlanCond::AnalyzePadA`.

A ladder's exits follow its tests, so a client exit order is a test order. Where the client emits
the both-zero answer, then the first-nonzero case's answers, then the last, write
`if (a == 0 && b == 0)` first, then `if (a != 0)` with its nested answers, then the last answer.
Example: `PlanCond::ClosestTargetToProjectile`. A spelling measured while another fault rotates
the registers reads lower than it is, so re-measure it once that fault is gone.

Move a read used in both arms into each arm. A read hoisted above the branch keeps its source
register alive across the test.

A pronoun arm that extends the word from memory wants the neighbouring reads after the
`if`/`else` (`movsx ecx, [param]` / `je` / `movsx ecx, word ptr [eax+4]`). Read above it, they
keep the tag in a register, and VC6 extends the low half of that register. Example:
`PlanAction::ModifyDamage`.

A null arm that sits among the inner arms and jumps past every test of the pointer is a lookup
written on its own: an `if`/`else` that assigns null, then `if (p != 0 && ...)` after it. Nested
inside the in-range arm, the null arm lands first. Example: `PlanAction::SetAnimation`.

### Early exits and flags

Write a search whose miss returns as an inline helper, not a `goto` or an inner `return`. VC6
treats a branch whose only statement is a `goto` as cold and moves its target past the function,
and merges identical `return 0` tails. An inline helper that returns 1 on a hit and 0 after the
loop costs no instruction: VC6 threads the constant into the caller's test, and the blocks land
in source order. Found-flags do the same and are banned. Example, from
`GameEngine::ParseTeamHTMFile`:

```cpp
inline int GameEngine::FindTeamDefinition(FileObj* pFile, char** ppLine, char* pszToken)
{
    do
    {
        if (Match(*ppLine, pszToken))
        {
            return 1;
        }
    } while (pFile->GetNextLine(ppLine) > 0);

    return 0;
}
```

A loop the client tests at the head every pass wants both conditions in the head.
`while (nLocked != 0 && nGuard != 0)`, with the flag started at one, stops VC6 rotating the guard
test to the bottom. A `do`/`while`, a trailing `break`, `for (;;)` and a `goto` loop all rotate.

Where the client zeroes a register above an allocation, use a result flag: `nResult = 0;` before
the allocation, `nResult = 1;` in the body, and one `return nResult;`. A `return 1` on the success
path lets VC6 prove the other path zero and fold the variable. Measure the whole function. Where
the client keeps a default (`or eax, -1`) in `eax` above a search loop, the found block sets the
same variable and breaks out. Example: `KeyboardDeviceObj::HandleKeyUp`.

Code taken from a DirectX SDK sample keeps the sample's error tail. Where the client sets a code
in each failure arm (`mov esi, 0xe102` / `jmp`) and has one return, write each failure as
`nResult = code; goto error;` and let success `goto` past the tail, as `src/sound/wave.cpp` does.

One out-of-line `return 0` block that several arms jump to is each arm's exit written as the last
`else`: `else if (dRoot > AXIS_EPS_LOWER) { dRoot = 0.0; } else { return 0; }`. The nested form,
`if (dRoot <= AXIS_EPS_LOWER) { return 0; }` before the fall-through, keeps a separate exit in
each arm. Example: `CollisionEllipsoid::IntersectRay`.

A test the client threads past on a search loop's exits reads the variable the found arm sets.
Where the found arm stores `nFound = i`, write `if (nFound == nHost)`, not `if (i == nHost)`. On
the exits VC6 knows the untouched variable and drops the test; the constant the client carries
through the join is VC6's value numbering. Example: `GameSession::HandlePlayerDisconnect`.

A flat early return is the null-guard shape. `if (p == 0) { return 0; }` costs nothing because
the pointer is already the zero; `if (p != 0) { ...; return 1; } return 0;` pays an
`xor eax, eax`.

VC6 rejects a bare `return;` in a non-`void` function (C2561) and a function with no return
statement (C4716). C4715, a function that falls off its end, is a decode error where the client's
other path never writes `eax` after the tested call: write `if (nErr == 0) { ... } return nErr;`.
Example: `JoystickMgr::SetThreshold`.

### Loops

VC6 rotates a top-tested loop itself, so write a plain `while` or `for`. A hand-rotated
`if (n > 0) { do ... while }` is usually worse, and a count guard around a `do`/`while` peels the
first pass. A `for` beats a `do`/`while` where the client falls into the loop-exit epilogue.

Leave a loop with `break`, not `goto`. A `for (;;)` with a `break` exit rotates differently from
the same loop with a `goto`. `continue` collapses a nested rotation.

Index the array; don't walk a pointer. A hand-walked pointer is loop-invariant, so VC6 hoists its
setup above the guard; with an index, VC6 builds the induction variable after the guard. The
client's `dec`/`jnz` does not mean the source counted down. The walker also changes the register
choice for every value near the loop, so a residue that looks like allocation can be a walk.
Example: `KeyboardDeviceObj::HandleKeyDown`.

#### Rotation and the loop test

A redundant test at the foot stops the rotation. Where the client has a forward branch and a
`jmp` back, repeat the test as `if (!cond) break;` at the end of the body. Nothing on the path in
proves it, so VC6 keeps it.

A guard above a loop that the client repeats at the foot, with the other exits at the head, is the
loop's own condition:
`while (*p != STREAM_EOF) { if (*p == '\n' || *p == '\r') break; ... }`. A guard `if` round a
`while` on the other exits rotates those instead. Example: `ParseUtil::ReadLine`.

A loop the client tests at the head and closes with a `jmp` back has an inline predicate as its
condition. VC6 rotates `while (a && b && c)`, `for (;;)` with `break`s and a `goto` loop alike,
but not `while (m_pCurrent->CanPassOn())`. Example: `PlanState::ActivateBehavior`.

`mov cx, ax` / `dec eax` / `test cx, cx` / `jle` is a post-decrement test,
`i = n; while (i-- > 0)`. An `if (n > 0)` guard round a `for` from `n - 1` adds a second test.
Example: `AnimPlayer::ComputeScale`.

A `for` whose body starts with `if (p == 0) break;` keeps the head test on entry. A `do`/`while`
with the same `break` lets VC6 prove the pointer non-null from the test above the loop and enter
below it. Example: `SaveObject::ParseGauntletConfig`.

A loop whose error arm and whose success compare both jump back to the head is
`do { n = Read(); if (n > 0) { ... } } while (n > 0);`. A `for (;;)` with a `break`, or a
`while` with the read in its condition, rotates and copies the read to the foot of the error arm.
Example: `GameApp::CopyCDFile`.

A prologue push can be a loop's shape. Two live ranges over the counter put the push in the
prologue; one range sinks it into the loop. A count-down `for` that passes `nRow - 1` lets VC6
common the subtraction with the decrement, `lea edi, [eax-1]`, and gives two ranges.

#### Counters and index types

A down-counter beside a byte-offset walker is a `char` index. Where the client clears an array
with `mov ecx, N` ... `add eax, 4` ... `dec ecx; jne` and this build compares the offset
(`cmp eax, N*4; jl`), the index was `char` or `unsigned char`. `short`, `unsigned int` and `long`
all compare the offset. Example: `GameEngine::Cmd_SetRatings`.

A counter the client copies (`mov eax, esi`) before it adds a loaded base is a `for` loop's
variable, `for (n = 0; n < k; n++)`. A `do`/`while` over the same counter adds in place. Example:
`ChampionshipSetupUIScreen::HideBracketTorches`.

A count added once before a loop can be an increment in it. Where the client does
`movsx edx, dx` / `add ebx, edx` ahead of an indexed loop over that many items, try `nTotal++` in
the body; VC6 reduces it to one add of the trip count. Example:
`EngineObject::ComputeBoundingSphere`.

A negated stride in its own slot and a trip count stored over a dead local is a row index. Write
`for (nRow = 0; nRow < cy; nRow++)` and `pSrc = pPixels + (cy - nRow - 1) * nStride;`, and
subscript the row table, `pBlock[nRow + 2]`. VC6 makes the negated stride, the row pointer and the
count itself. Example: `SoftTex::CompressToRLE`.

A subscript cast to `short` keeps the index. Where the client loops on an `int` but addresses
each store through `movsx ecx, di`, write `a[(short)i]`. VC6 does not strength-reduce the cast
subscript. Example: `KeyboardDeviceObj::KeyboardDeviceObj`.

A value stepped in the `for` header steps before the cursors VC6 derives. When the client steps
it among them, write it as an expression of the counter, such as `nTeam + i * 2`. Example:
`GameEngine::SubstituteRecoveredReserve`.

The comma operator orders a `for` header. The increment clause is always emitted last, so a comma
there can order steps a plain `for` cannot. Put the pointer step in the header and the count step
in the body when the client holds the pointer in `esi`.

#### Cursors and walked records

Two arrays walked by one index share one cursor, and the base is the array read last. VC6
strength-reduces both subscripts onto one register. When the residue is one `lea` base and a pair
of displacements that add up, swap the two reads (`GameEngine::SyncPlayerDataToTeam`):

```cpp
nSlot = pTeam->aSeat[nSeat];           // read first: reached by the difference
pPlayer = pTeam->apOnIce[nSeat];       // read last: the base
```

Two arrays of one local struct reached from two bases, one of them spilled, are subscripted at
every use: `block.teamOne.aSeat[i]` in the test and again in the call. A named copy of either
element lets VC6 reach both from one base. Example: `GameEngine::ReadSaveFile`.

A row index written as a product keeps the client's unreduced store,
`a[nRound * N + nSlot]`. A base the source steps itself, `nBase += N`, becomes a walking pointer
that spills and grows the frame. Example: `SaveObject::ParseGauntletConfig`.

`lea` of `(i + K) * stride` with no displacement, then `add eax, off`, is a row pointer
subscripted by the loop counter: `pRow = m_aRows[i];`, then `pRow[nAxis * 3 + k]`. A pointer the
source steps, `pRow += 3`, folds the offset into the last `lea`. Example:
`ParticleSystem::ApplyToMesh`.

A copy loop whose pointers start as fresh registers is subscripted:
`((int*)pTo)[i + 1] = ((int*)pFrom)[i];`. A pointer the source steps itself has no copy. Example:
`GameEngine::SetDifficultyLevel`.

A loop pointer that addresses one field at `[reg]` and the rest at negative offsets is a record
pointer; VC6 rebases a walked struct pointer onto a field it reads often. Declare the record and
walk a copy of the allocation by type, `pGroup = pTable;` then `pGroup++`. Walking the pointer the
header stores go through leaves a dead induction variable in the frame. Example:
`KeyboardDeviceObj::BuildActionBindingTable`.

A cursor that sits on the field the loop reads most, where a walked record pointer does not, is an
array subscripted by the counter: `m_aParticles[nIndex].m_flLife` at each use. VC6 rebases its own
induction pointer and passes `lea ecx, [edi - 0x1c]` as `this`. Example: `ParticleSystem::Update`.

`mov edx, esi` / `lea esi, [dst]` / `sub edx, esi` before a copy loop is a plain two-pointer copy,
`while (*pszSource != 0) { *pszDest++ = *pszSource++; }`. VC6 makes the fixed distance itself.
Example: `FileObj::Open`.

`lea ebx, [x + off]` and `mov ecx, [x + off]` from one base, then the result cleared, is
`for (; (pAt = *ppLink) != 0; ppLink = &pAt->m_pNext)` with a `break` inside, one `return` after
the loop, and the result cleared after `ppLink` is set. Example:
`CD3DTextureManager::DeactivateMaterialLookup`.

A hit that takes its own reference and returns inside the walk is an early `return`:
`pEntry->m_wRefs++; return pEntry;` in the loop, and the new entry gets its own local. Example:
`ModelDatabase::FindOrLoadTexture`.

`mov edi, [edi]` after a call, where `edi` held the slot's address, is a slot array read by index:
`while (m_apSlots[n] != 0) { m_apSlots[n]->Bind(...); pNode = m_apSlots[n]; n = n + 1; pNode->m_bPersistent = 1; }`.
A walked slot pointer, or a node carried in a local, puts the node in another register. Where the
client loads after the `lea`, the first node comes from the allocation and the loop tests `pNode`.
Example: the `BehObj_*::Init` rows.

### Tails, merges and cross-jumps

VC6 merges identical tails and cross-jumps identical suffixes, but not always where the client's
compiler did.

VC6 tail-merges in every function of a translation unit except the first one it optimises.
Where the first function in a unit needs a merge the client has, give the shared tail external
linkage, so an out-of-line copy is emitted ahead of both, or put another function in front. A
lever that depends on being first stops working when a body moves into a larger file.

VC6 does not duplicate a block, so a block duplicated in the client is duplicated in the source.
Write the call, the copy and the `return` in every arm. VC6 merges two written copies, but not a
copy and a jump. The exception is a block VC6 duplicates itself after constant propagation: if it
is the function's last, a hand-written copy folds and the compiler's own does not, so write it
once.

Copy the client's statements, not the decompiler's variables. Unrolling what reads as a loop,
writing a helper out three times, or putting the second half of a function in the first half's
`else` all pay where the client did that.

A load between the branch and a shared store stops VC6 hoisting the store. A partial tail merge
past a load means VC6 hoisted the load; write the shared tail inside the other arm.

Return blocks merge by the instruction in front of them. A guard that skips a case's body jumps to
that case's `return` block, which merges only with earlier ones whose body ends in the same
instruction, such as `call edi`. A `return` written inside the guarded block as well sends the
guard to the function's first return block. `KeyConfigDialog::OnCommand` shows both.

#### Where the client merges less

Give each arm its own local where the client merges the arms only from the `call`. With one
shared struct, VC6 merges each arm's address `lea` and constant pushes into one tail. Example:
`Display::ChangeColorDepth`.

A cleanup call whose arguments each arm pushes before a `jmp` to one shared `call` is written in
each arm. Written once after the `if`/`else`, VC6 shares the whole call and ends the callee-saved
registers' live ranges early. Example: `GDI::DrawBitmapToWindow`.

Three copies defeat a merge of two. Where the client emits a block twice and VC6 merges this
build's two copies, write a third copy. VC6 picks each copy's register to avoid the one its arm
defined last, and blocks with different registers do not cross-jump.

Move the zero exit off the end. VC6 makes the function's last `return 0` the survivor and jumps
guards into it. Where the client keeps an epilogue per arm, write the stores and the `return 1`
at the end of each arm, so the zero exit is no longer last. This holds only where the shared
block is the function's own last block and the guards are zero exits.

Write the last guard flat where VC6 merges returns the client keeps: `if (n == 0) return 0;` then
the body, not the body nested in `if (n != 0)`.

Give each arm its own copy of a shared tail and a `return`. VC6 hands out scratch registers in
code generation order. A `*pResult = 1;` after an `else` is generated last and rotates every
register in the arm before it.

Two arms that free the same pointer want a null test each. VC6 otherwise hoists an identical
`push` above the branch and folds the stack cleanups:

```cpp
if (p != 0)
{
    ::operator delete(p);
}
```

Split one gate into two `if`s with a store each to remove a cross-jump the diff blames on the
allocator.

#### Where the client merges more

A `jmp` into the other arm's `jne`, above one answer block, is two answer blocks. Write
`if (a == 6) { if (b == 6) { answer; return; } } else if (a == 0x36 && b == 0x36) { answer; return; }`.
VC6 merges the two answers and cross-jumps the first arm's test into the second arm's branch.
`goto`s to one answer block give each test its own branch. Example: `PlanCond::TakeDamage`.

A tail the client repeats in each arm with other scratch registers is written once after the
join. VC6 copies a short final statement into each arm itself, and its copies take other
registers than two written copies do. Where this build's copies differ from the client's only in
`eax`, `ecx` and `edx`, write the statement once after the `if`/`else`. Example:
`AgendaItem::Detach`.

Where the client shares a block and this build duplicates it, nest the arms. A backward `goto`
does not buy the sharing back.

One push sequence reached by a `jmp` from two arms is one call. Compute the one differing
argument in an `if`/`else` into a local and write the call once; two written calls merge only
their last pushes. Example: `AnimScriptTable::ParseScriptLine`.

A flag test and an object load in one register, then one call through it, is the call written in
each arm: `if (flag) { a->F(x); } else { b->F(x); }`. VC6 merges the two calls and keeps the
select in the register the flag used. A local chosen first and called once swaps the registers.
Example: `DDraw::ReleaseBackBufferDC`.

A null arm that loads the tail's pointer, tests it and jumps into the tail's call is the tail
written in an early return too:
`if (p == 0) { pNext = m_pNext; if (pNext != 0) pNext->Process(nArg); return; }` above the body.
VC6 merges the two calls and keeps both tests. Example: `CHeadTrack::Process`.

A guard that jumps into a middle case's return is an early return above the switch:
`if (g_cheatState.ProcessKeypress((char)nKey)) { return 1; }`, then the `switch`. The switch
inside `if (!...)` with `return 1;` after it jumps to the last case's tail. Example:
`GameApp::HandleMatchKeypress`.

### Statement order

The order of two independent statements is reachable. Ask whether one source statement could
have produced both instructions. Inside one statement's expansion, the order is the scheduler's.
Across two statements, swap them, if they may legally swap; a store group and the virtual call
that must follow it cannot. Moving a read above `sqrt` can hoist a virtual call with it.

The same instruction in different blocks is a source fact: no scheduler moves an instruction
between blocks. An unwind state store in the wrong block means the client finishes constructing
an object at a different point.

Two independent field stores emit in reverse source order, and the field the source writes first
takes `eax`. VC6 stores fields through `eax`, `ecx`, `edx`, then `eax` again. Read the registers
backwards for the source order.

A virtual call through a named pointer lets VC6 sink the store before it. With
`pPanel = &m_oMeter;`, `pPanel->m_nFrame = n; pPanel->Draw();` stores after the vtable load. Where
the client stores first, call through the member, `m_oMeter.Draw();`. Example:
`InfoBar::DrawScene`.

A flag `or` in a callee-saved register that is reloaded next is a compound assignment, written
first: `m_nFlags |= FLAG;` above the address computations. A named copy of the member, or the
`|=` after them, puts the flags in `eax` and narrows the `or` to `al`. Example:
`EngineObject::UpdateRenderSlotTransform`.

Sweep a run of plain stores by script. VC6 schedules the loads of a store run ahead of the stores,
so the listing seldom shows the source order. Probe every permutation of a run of independent
assignments and compare each listing with the client's. Take in every statement of the run,
including one a blank line sets apart: the five zero stores of `Game::InitMatchMarkers` match
only with the sixth one above them in the sweep.

A sweep finds frame shifts as well as real orders. Most large gains put a load or store where the
client does not have it, so check in the client's listing that the moved access now sits where
the client has it. A sweep scored on probe residue also rewards moves that read a value before the
statement that writes it, or that call two functions in the other order, so check each hit's data
flow too: across 47 retired rows, every hit failed one of the two checks.

By-value getter temporaries are built in source order. Where the client copies `GetVel()` answers
into frame temporaries, write the operands in the order of its copies (`Player::FindBungeeVictim`).

A table entry filled from two running counts takes both values before the copy. Where the client
holds the old count in a frame dword across a `strcpy`, write
`const int nValue = nFileSlot; nFileSlot++; PlanOpcodeRef* const pSymbol = &pTable[nSlot]; nSlot++;`,
then the copy and `pSymbol->nValue = nValue;`. Example: `PlanVM::LoadSymbolTables`. A clean-up
that folds such locals away can cost a row like this, so check the commit that dropped them before
you accept its wall.

A copy written out of order to fix one hunk can be what is left. `CSetAttitude`'s forward column
copied z, x, y keeps `Process` and `Release` off; written x, y, z, both match. Re-read a retired
row's oddly ordered statements before you accept its evidence.

## x87 and float loads

The commonest float residue is two operands loaded in the other order: `fld A` / `fmul B` against
`fld B` / `fmul A`. VC6 sorts the leaves of a commutative float expression, so source order alone
rarely decides. The rules below override the sort.

### The default leaf order

With nothing parenthesised, VC6 orders the two leaves of `+` or `*` as follows. Source order
decides none of these:

- two members of one object: the larger offset first;
- a member against a plain local, parameter or global: the plain one first;
- two locals: by frame slot;
- two members of different objects: the object whose pointer was defined first;
- two globals: ascending by address, unless a common subexpression escapes the sort;
- a named global ranks ahead of a literal. A `0.0f` literal becomes `fldz`, so a client that
  loads a zero from memory read a variable: name it for what it holds.

For a sum of more than two terms:

- member reads of one object come out by descending offset;
- named locals, or a record filled by one structure assignment, come out in reverse source order;
- each vector read through its own pointer follows the source;
- `x + (y + z)` computes the innermost pair first;
- a flat three-term sum sorts ascending, and an explicit pair sorts descending.

### Getters round one operand

Parentheses round a whole product choose which product of a sum comes first. They are ordinary
style, and since VC6 honours them they show the original had them, so keep them. Where fixing one
leaf flips products far away in the same statement, bracket the pair. Parentheses round a lone
operand are steering and stay out: VC6 loads a parenthesised leaf first when it is a dereference
at a nonzero offset, but the original did not write them.

A leaf read through an inline const getter loads first. A float returned by value is ranked ahead
of the other operand, such as `GetElement(nRow, nCol)` on `TransformObject`. A whole `Vec3`
cannot be read this way: a `const Vec3&` return emits the plain field read, and a `Vec3` by value
copies the vector. One component can: a member `Vec3` takes a float getter of its owner, such as
`GetSurfNormalX()` on `PhysicsBody`, and a local `Vec3` or one reached through a `Vec3*` takes
`Vec3::GetY()` and `GetZ()`. At offset zero `GetX()` does not promote its own operand, but on one
operand of an `x * x` product it turns the order: `pAccum->GetX() * pLine->x` loads the line's `x`
first (`CAvoidObjects::Process`).

A getter also ranks its whole product: in a sum of products, the product holding the getter is
evaluated first. Where the client loads the leaf first but adds its product last, bracket the
other two products and put the getter's product after them: `(a * x + b * z) + c * v.GetY()`, as
in `CameraProjectionState::RotateX`. A getter leaves the scheduler alone: it can fix the order of
an `FADD` and still move a nearby rebase above it.

A divisor read through a getter loads first. Where the client does `fld [p+X]` / `fdivr a`, write
`a / p->GetM()`. Example: `CTrackTraj::InitComplexRange`.

Integer getters don't reorder arithmetic. The lever is x87 only, and it does not reach a
comparison or an integer sum (see [Integer operand order](#known-walls)). A pointer getter can
still move a load: `GameEngine::GetSlot` in place of `m_ppSlots[n]` loads each object before the
float argument's store in `PlanCond::AnalyzePadA`.

Sweep the sites, one per function per build, and keep a getter only where its row rises. Over
nearly four hundred sites, this matched `ObjectStats::UpdateMomentumAndEnergy` and moved eight
more rows. Most sites are neutral and many lose. Each new getter's header can move the
[header-sensitive rows](#header-sensitive-rows) on its own.

A leaf in the wrong order can look like a scheduler wall, with what look like two scheduling slots
and two unpaired epilogue lines. Where the partner operand is already in `st(0)`, VC6 folds the
field into `fmul [eax+0xa4]`, and the client's `fld` / `fmul st(1)` / `fstp st(0)` have nothing to
pair with. Try a getter first.

Where the client re-reads a member at every site, write the computed address inline rather than
naming the pointer. A named pointer costs an `add` and a displacement the client does not have.

A split product ranks a value already on the x87 stack. `v = flDot; v *= x;` gives the client's
`fld st(0)` / `fmul [x]`, where `flDot * x` loads `x` and multiplies by `st(1)`. Example:
`CGlideOnSurface::Release`.

Write an identity only where the client performs it. `x * 1.0f` folds away and only reranks the
leaves, which steers bytes with an operation the original does not have. Where the client has an
`fmul` by a 1.0 constant, name the constant for what it scales, under a `GLOBAL` marker.

### Other ways out of the sort

Use the vector's own members for a vector sum or clear. Where three field sums of two local
`Vec3`s come out in another leaf order than the client's, `v.Add(vDelta)` and
`v.Set(0.0f, 0.0f, 0.0f)` can give it. Measure the two together: in `PlanCond::AmIKnockedLoose`
each alone is neutral, and only the pair raises the row.

Split the expression into statements. One float operator per statement removes the sort:
`t = R; t *= L; acc = t;`, or `float flHigh = g_a; flHigh += g_b;` for a two-term sum. Split
`x * a * b` with two constants, or VC6 folds them. A split must match the client's first pair
exactly.

Assign named float locals in the client's load order. Where several differences feed one sum,
this gives that order at no frame cost, if the names own a frame slot already.

A value the client keeps in `st(1)` under several products, popped with `fstp st(0)` after the
last, is a `const double` local. A `float` local gives `fld st(0)` / `fmul [x]` and lets the last
product consume it. Store it back with an explicit `(float)`. Example:
`ColorPalette::BuildFadeTable`.

A `double` local for one addend defeats two-term canonicalisation; a `float` local is stored and
reloaded and does not help. A `double` local for `float * int` gives `fld` / `fimul` in place of
`fild` / `fmul`. For two named globals, `dDegToRad = g_flDegToRad;` then
`(float)(dDegToRad * g_flLightAzimuth54)` loads the first global first. Example:
`Renderer::InitializeLightDirections`.

Accumulate with `+=`. `m += x` loads `x` first; `m = x + m` loads the cell first.

VC6 commutes the first `fadd` of a basic block. Exactly one `fadd` in a run flips. Read the
destination into a local first, `float a = g_x; g_x = a + g_c;`. If a call splits the block, write
a copy statement and a sum statement.

Name the operand the client loads in a `const` local, never the one it folds.
`const float flZ = v.z;` then `v.z = flZ + g_flConstant;` puts the local on top. For a pointer
dereference, `(float)((double)*pPtr + g_flConstant)`, named in a float and stored on its own line,
does the same.

A `(double)` on one side of a float compare chooses which operand loads first. `(double)flA < flB`
reproduces the client's order. The tree writes the plain compare anyway, and dropping the cast
from five sites cost four exact rows, among them `Vec3::FindClosest`.

### Literals and named constants

VC6 defers a float literal to the operation. `0.5f * x` always emits `fld x` / `fmul [0.5]`, so a
client `fld` of a constant means the source read a variable there. More folds:

- `x * 2.0f` becomes `fadd st(0), st(0)`;
- `x / 3.0f` becomes a multiply by the reciprocal, so a client `fdiv [const]` means a named
  constant;
- `-2.0f` folds to one `fld`, losing the client's `fld 2.0` / `fchs`;
- `x * k + x` becomes `(k + 1) * x`, even across statements. Split the product into its own
  statement; where VC6 still folds it, the row is a wall for now;
- `(9.0f - 1.0f) * 0.5f` folds to one value. Where the client does `fld [9.0]` then `fsub [1.0]`,
  both operands are `.rdata` variables, even where the CSV names them as constants (see below);
- a literal folds through a clamp whose arm's own copy of the tail multiplies it
  (`1.0f * 0.08f + 0.65f` to 0.73). Write the clamp as arms that only assign, with one shared
  tail below them (`PlanAction::LaunchAt`);
- `sin(0.0)` and `cos(0.0)` are not folded. `sin` and `cos` are intrinsic (`fsin`, `fcos`);
  `floor` is a call followed by `__ftol`.

A named constant behaves differently from a literal:

- VC6 does not fold an `extern const float` defined in another unit, so it stays a separate load.
  A `const float` local, and an inline function that returns a literal, fold like literals.
- A namespace-scope `const float` with its initialiser in a header does not fold either. Each
  unit gets its own copy and loads it first, like an `extern const float`. It cannot carry a
  marker: a `const` at namespace scope has internal linkage, so a marked constant needs `extern`.
- VC6 loads an `extern const float` operand before the other one, and a literal after it. Where
  the client loads a local and then multiplies or adds the named constant, that unit read it as a
  literal or as a variable that is not `const`. One .rdata address read by many unrelated units
  is a pooled literal, so write the literal at each site and give the address a CSV row. The 6.0
  at 0x004b1480 has sixty reads; as a named global it cost three rows, and as `6.0f` it makes
  `Object::SpawnAtObjectRotated` exact.

The width of a literal follows the expression:

- A literal in a `(double)` sum is a `qword` constant. A cast or a `double` local that steered a
  named float's load order turns the literal's `fadd dword` into `fadd qword`; drop the cast when
  the name becomes a literal.
- A float literal compared with a `fabs` result is a `qword` constant, `fabs(x) < 0.001f`. A
  `(float)` on the `fabs` makes the compare `dword`.

`symbols/float_constants.csv` decides how a constant pairs. A literal pairs with the original's
constant if and only if the CSV has a row for its address. A row and a `GLOBAL` marker on the
same address never pair: the report shows `fld [2.0 (FLOAT)]` against `[g (DATA)]`. Decide by
what the client does with the load:

- arithmetic with the constant as an operand: keep the row and write a literal;
- a bare `fld` followed by a negation or a divide, or a `mov` into a field: declare an
  `extern const float` with a definition in another unit, under a `GLOBAL` marker, and remove the
  row;
- both in one image: declare a name without a marker, so the row still serves the literal sites.

Where every reader can name the global, mark it. Ask Ghidra for the address's readers
(`get_xrefs_to`). If the literal sites are compares or other arithmetic, write the global there
too: `fcomp [g]` compiles the same as `fcomp [literal]`. Then give the name a `GLOBAL` marker and
remove the CSV row, and the `mov` sites pair (`SlerpSeg::SlerpSeg`). An unfolded constant
expression such as `fld [9.0]` / `fsub [1.0]`, read only there, takes a `GLOBAL` marker on each
address in place of its CSV row.

### Copies, spills and stores

A float copied by value is an integer `mov` pair, the same bytes an `int` pun gives, so write the
typed assignment. A float literal store folds to an integer immediate too; route the constant
through the named float the client has.

A float the client copies into a field with `FLD`/`FSTP` went through an inline float return or
parameter. Read the source through a by-value getter or store through a setter:
`CameraObject::SetViewFracX(g_flCamFracFull)` in `CHeadTrack::Process`. A local passed to the
same setter keeps its integer moves, and so does a getter copied into a local. A local vector
filled word by word through the x87 stack is the same lever:
`aTarget.Set(pWorld->m_a[0][3], pWorld->m_a[1][3], pWorld->m_a[2][3]);` in `CTrack::Process`.

A value the client stores with `fstp` and reloads is a split statement: `a = b * c; a = a * d;`.
Where VC6 still forwards the value, the row is a wall for now. To get VC6 to fill the gap between
`fcom` and `fnstsw`, write the store in both arms.

A store the client makes at a join and reloads is written in every arm. VC6 merges the stores
into the join and cannot forward the value past it. Example: `AnimPlayer::UpdateFrame`.

Store a call's float result through a named `float` local to delay the `fstp` to where the client
has it.

A float argument the client passes with `fld` and `fstp [esp]` is a `const float` local. A float
global passed directly goes out with an integer `mov` and `push`. Name the value first,
`const float flLight = g_flZeroSWRend;`, and pass the name.

A temporary that lives only on the x87 stack is `double`. Ghidra shows it as `float10`.

One name per overlapping live range. Floats with disjoint ranges may share one name. A float used
in both arms of a branch must not be named: write the product in both arms and let VC6 common it,
or you get `fadd` and `fstp st(0)` where the client has `faddp`. The last defined of several
competing floats stays in `st(0)`. `fld st(0)` then `fcomp` means a copy local, a second name used
only for the comparison. A run of `fld`s with no store between them is an inline call's
arguments.

#### Vectors and structure copies

Stores the client keeps for a cross product that this build forwards are read through an inline
helper that takes the vectors' addresses. Written out as `a.x * b.y - a.y * b.x`, VC6 keeps the
edges on the x87 stack and drops their stores; `Cross2D(&v2, &v1)` keeps them. Example:
`Face::BackfaceCullAndClipCheck`.

Products the client makes before the first add, keeping the first on the x87 stack and storing the
others to a frame temporary, are a scaled `Vec3`:
`const Vec3 vStep = pRow[nAxis].Scaled(flRatio);`, with its components added to the base in a
second statement (`ParticleSystem::ApplyToMesh`). Where the adds go into a vector reached through
a member pointer the client loads once, add through an inline member,
`m_pVel->Add(vGap.Scaled(0.25f));` (`CTrack::Process`). The same holds for a sum the client keeps
partly on the x87 stack before subtracting a third vector: sum into a block `Vec3` first and
subtract from it in a second run of statements. Written as `(a.x + b.x) - c->x`, VC6 stores
nothing.

A structure copy that this build forwards, where the client reads the copy back, is
`Set(v.x, v.y, v.z)`. After `vScratch = vLocal;`, VC6 can read `vLocal` at each later use of
`vScratch`, which also moves `vLocal` up the slot ranking.
`vScratch.Set(vLocal.x, vLocal.y, vLocal.z)` keeps the reads on the copy
(`CollisionCheck::DromerRampCollision`). Where the build already reads the copy, `Set` costs.

`add reg, K` and then `[reg]`, `[reg+4]`, `[reg+8]` after a call is a structure copy from the
field, `vTriple = pBody->vel;`. Read through a named `float*`, the offset folds into each load.
Example: `PlanCond::DecideGoalieLaunch`.

A record parameter the client copies into a member is one structure assignment; two member stores
give the scheduler two statements to place. Where the member's class adds its own inline
`operator=`, call the base's, `m_ptPosition.Point::operator=(pt);`: the derived operator keeps
both stores where the source puts them, and the base's lets the next statement's member load in
between them. Example: `CDDBitMap::SetPosition`.

Stores the client keeps into a local that later writes make dead are a structure copy. VC6 deletes
a dead scalar or pun store, but keeps every store of a structure assignment: `vNormal = vPoint;`.
Example: `CollisionCheck::ComputeNearestNormal`.

Two words of a returned record held in callee-saved registers across the next call are a record
copy: `amounts = GetInset();`, then pass `amounts.x, amounts.y`. Example: `HUD::GetHeadCamSize`.

Words the client keeps in registers for two copies come from a local whose address is never
taken: `const Vec3 vHead = vScratch;` then `vOrigin = vHead;` and `pBody->pos = vHead;`. A store
through a pointer can alias any local whose address escapes, so VC6 reloads from it. Example:
`PlanAction::LoseHead`.

A store the client sinks below later loads lives outside any record whose address is taken. VC6
keeps a store into an object whose address the function takes where the source puts it. Where one
local struct holds both kinds, split it into one `Vec3` for each vector. Example:
`CSteerToward::Process`.

A member the client writes through a register was reached through a pointer. VC6 addresses a
member written by name as `[this + offset]`, even when it holds the member's address for a call.
Where the client reaches a component through `[reg]` or `[reg + 4]`, write it through
`Vec3* const pNormal = &m_vNormal;`. Without the pointer `PhysicsBody::IntegrateMotion` loses 17
points. Keep such a pointer with one line citing the client's addresses.

`mov eax, [eax]` and a store to a slot straight after a call is a copy out of the returned
pointer: `v = *f(&tmp, ...);`, not a call and then `v = tmp;`. Declare `tmp` and `v` in the block
that uses them, so their slots can share. Example: `EngineObject::InitModel`.

`add reg, K` and a dword and word moved through `[reg]` is a six-byte `memcpy`. Two typed stores
fold the displacement into each access. Example: `GameEngine::WriteSaveFile`.

A word the client writes at `[reg+4]` off a pointer to a packed record's first word, not off the
stream cursor, belongs to a nested record held by a named pointer:
`StationaryCommand* const pCommand = &pEvent->command;`. A flag the client ORs into a word already
in the stream is a read, an OR and a write of that word. Example:
`PlayerBase::NetUpdateMultiObject`.

`add eax, off` and three word copies into adjacent globals are a structure copy into one global
record. Declare the record, `Vec3 g_vRadarCameraDir;`, give it the first word's marker, and assign
it whole. Example: `Radar::Update`.

One `mov` pair per word with a rebase for each row is a structure assignment of a `Vec3` into a
`float[4]` row. Give the row a union type, `MatrixRow` in `math/transformobject.h`, whose
`float af[4]` keeps the variable-column reads and whose `Vec3 vAxis` takes the assignment.
Example: `AnimClip::GetBoneTransform`.

#### Working points

Stores the client keeps into a working point that nothing reads again mean the source wrote that
point again later. VC6 drops a struct copy into a local no later statement names, but keeps the
unread words while the local is still in use. In `InfoBar::LayoutSurfaces` the prompts below are
placed through `ptPos`; a separate `Point` for them drops the stores. Where the client also
reloads the source of such a copy, write the chain,
`m_sReticle.m_ptPosition = ptPos = GetHeadCamOrigin();`.

`mov edi, eax` straight after a position is computed is a copy of the working point. Compute into
the working local, set the member from it, then `const Point ptCamera = ptPos;`. A separate
`Point` computed first gives the client's values in other registers.

A member the client loads before the global it is subtracted from was copied into a local first.
`ptSize = m_tPrompt.GetSize();` then `(g_nBackBufferWidth - ptSize.x) / 2` loads the size first;
the member read inside the expression loads the global first.

#### Scratch vectors on the x87 stack

Two components of a vector stored to adjacent slots, with the first kept on the x87 stack and the
slots written again by a later vector, are one scratch `Vec3` assigned twice. VC6 forwards the
first component and drops its store, but keeps the other two. Values the client keeps on the x87
stack across statements are plain `float` scalars. Example: `CTrackTraj::Process`.

A projection the client folds on the dot in place is three stores into one vector. Where the
client stores the x and y of `flDot * vNormal`, then multiplies the dot by z and subtracts it
straight from the stack, write all three components, `vProj.z = flDot * vNormal.z;` and
`pDir->z -= vProj.z;` (`CGlideOnSurface::Release`). Where the subtraction goes into a local vector
whose new z the client keeps on the stack, store only x and y in the shared scratch vector and
subtract the z product in place: `vOrigin.z -= flDot * vLocal.z;`
(`PlanAction::ReflectOffWall`).

A field the client stores into its own pointer's frame slot is a `float` copy read once,
`const float flScaleX = pOffsetScales->x;`, used at both sites. The copy shares the dead pointer's
slot. Example: `CTrack::Process`.

### Comparisons

VC6 never commutes a float compare. Which side is written first decides the loaded operand. Put a
constant on the left for `fld` / `fcompp`, or on the right for `fcomp [mem]`. Put a call on the
left where the client calls first.

Read the status mask for the operator. `fnstsw` puts C0 in bit 0 and C3 in bit 6. `test ah, 1` is
a strict `<` or `>`. `test ah, 0x41` is `<=` or `>=`.

The width of the left operand decides the compare. A compare done in `double` gives `fld` / `fld`
/ `fcompp`. Narrow the left side with `(float)(...)` to get `fcomp`. Compare against an eight-byte
zero with `0.0`, not `0.0f`. For a double against a float global, don't cast the global.

A compare against an array element's first field goes to memory through a named element pointer.
`min(m_block.vMin.x, pVerts[nIndex].x)` uses `fcompp`; with
`const Vec3* const pVert = &pVerts[nIndex];`, `min(m_block.vMin.x, pVert->x)` gives the client's
`fcomp [reg]`. Example: `MeshBone::GetBoundingBox`.

A float copied with integer moves and loaded straight back for a compare is assigned in the
condition: `if ((flRate = p->flSpeed) < flLength)`. As two statements, VC6 forwards the field
into `fld` / `fst`. Example: `CRelaxToPosition::Process`.

A `bool` holding a float compare across an x87 block gives `mov al, 1` / `xor al, al`. A
cancelling `fxch st(1)` pair means a narrowing `(float)` cast on a binary operand.

### Where a value is rounded

A float result on the x87 stack carries the FPU's precision until it is stored to a `float` slot.
Where the client keeps a value live and this build stores and reloads it, or the reverse, the two
compute different results. A rewrite can raise the score while the result drifts from the
client's, so check every `fld st(n)`, `fst` and `fmul st(n)` the client has in the rewritten span.
`tools/x87_precision.py` lists the rows where this happens, ranked by the number of values that
differ. It is a heuristic: read the listing before you change the source.

The difference matters only at 53 bits. The CRT's float initialiser at 0x0049eb05 calls
`_controlfp(_PC_53, _MCW_PC)` through 0x004a3604, and no game code writes the control word. The
software renderer path stays at 53 bits. Direct3D drops the FPU to 24 bits when it creates its
device, because no `SetCooperativeLevel` call sets `DDSCL_FPUPRESERVE` or `DDSCL_FPUSETUP` (the
client passes 0x8 and 0x51 at 0x0043a6c0, 0x0043ae20 and 0x0043c310). At 24 bits a live result is
already a float. A difference on operands that are exact at either width changes nothing.

A sum the client keeps live is written as component stores into its `Vec3`. In
`CGlideOnSurface::UpdateTerrainPhysics` (0x0040c3cd to 0x0040c3e8),
`Vec3 vStep = TerrainStep::Sum(...)` stores x to the return temporary and reloads it;
`vStep.x = vDiff.x + m_pSmoothVel->x;` and the same for y and z keep x live, as the client does.

Compare from the side that is in memory. Where the client stores `flTarget` and reloads it,
`if (flTarget < *pSpeed)` forwards the live product with `fst`/`fcomp`, and
`if (*pSpeed > flTarget)` compares against the stored float, as the client does. NaN gives false
both ways.

A subtraction the client keeps live is a `const float` local:
`const float flZ = vOrigin.z - flDot * vLocal.z;` then `vOrigin.z = flZ * flScale;`.
`vOrigin.z -= ...` stores it, because `vOrigin` has its address taken. Example:
`PlanAction::ReflectOffWall` (0x00405793 to 0x004057c9).

A record copied whole keeps its stores. In `RampObject::InitCollisionPlanes` (0x00480093 to
0x004800a1) the client stores each edge's slope to one local, copies it into the member and later
reloads the slope. Written to the members, or through a local copied field by field, VC6 forwards
the live slope; `RampEdge edge;` copied with `m_edgeNearMin = edge;` gives the store and reload.

A vector written whole keeps the stores of its neighbours. In `SWRend::DrawFace` (0x0044a3fa to
0x0044a412) writing `vCorner0.y = vCorner2.y - vCorner0.y;` as well as the x, even though VC6
never stores it, makes VC6 store and reload both edge values as the client does. A `const float`
for the y does not. It costs some of the slot map, and stays for the rounding.

### Arguments and conversions

Route a float argument through a `double` local where the client has `push ecx` / `fld` /
`fstp [esp]` and this build has `mov reg, [..]` / `push reg`. The argument reserve moves every
displacement in the block, so this can be worth much more than it looks.

A `this` the client moves after the `fstp [esp]` of a float argument is a `const float` local in
its own block round the call. A `double` local, or the member read in the argument list, lets VC6
move the `this` first. Example: `CExplosionAnimation::Process`.

A float passed as `fld dword` then `fstp qword` into eight bytes is varargs promotion. Declare the
callee `f(fixed, ...)`.

`call __ftol` before a member call loads its object is an `int` assigned in both arms,
`nPos = (int)(...)`, then passed. A `(int)` cast in the argument list comes after the object and
its vtable load. Example: `CDisplayPassBar::Process`.

An unsigned cast has a tell: a stray `mov [esp+N], 0` beside an x87 divide. VC6 converts an
unsigned value through a 64-bit temporary, writes a zero high word, and peepholes the divide back
to a dword `fidiv`, leaving both stores. `(float)(unsigned int)n` is a conversion, not a doubled
cast, and converts differently from a signed value; keep it.

`sub esp, 0xc` then `mov reg, esp` before a call is a twelve-byte struct passed by value. The
`ret` immediate is the same as for three floats, so only the caller shows it.

A `this` `lea` between the two pushes of one record's fields is the record passed by value:
`origin.Offset(pSource->GetInset());`. Two ints, a local copy, or an inline by-value wrapper all
put the `lea` after the pushes. Where other callers pass two ints to the same address, the
original had both overloads and its link folded them, so the call line cannot pair. Example:
`HUD::SetReticlePos`.

A record held in two registers and pushed by value with no constructor call is a converting
constructor that takes the record by value: `AllocSurface(SurfacePoint(size), 1, 1)` with
`SurfacePoint(Point)`. A `const Point&` constructor reads the fields back from the local's slot.
Example: `Radar::Init`.

`mov eax, [reg]` then `push eax` from a pointer parameter is a `const String&` read through its
conversion operator, not a `const char**`. Example: `AnimScriptTable::ParseAndAddAnimDB`.

`push eax` of the previous call's result, with the temporaries freed in reverse at the end, is one
nested expression. Named temporaries push a `lea` of each instead. A pointer parameter that takes
the nested result was a reference: `String::Concat` is MFC's `operator+`, which takes a
`const CString&`. Example: `ModelFile::Open`.

## Frame layout

A frame fault shows as `[esp+N]` or `[ebp-N]` displacements that differ while the instructions
match. Classify it first with `python tools/slot_residue.py`. Its `kind` column separates:

| Kind | Meaning | What reaches it |
|---|---|---|
| `size` | the two builds disagree on `sub esp, K` | a missing or extra local, or an aggregate |
| `perm` | deltas of both signs: slots trade places | records, see below |
| `push` | one side has a callee-saved push the other lacks | a live range |
| `shift` | one-signed delta with neither tell | usually a push or a size fault read wrongly |

A single uniform delta is the shape least likely to be a slot order: one sign means one thing
moved the whole frame.

### Frame size

A four-byte excess is one spilled value, never padding: shorten the value's live range.

VC6 deletes an unreferenced local (C4101), and `(void)x;` does not count as a reference. So an
unexplained client dword is an aggregate member nothing writes, a wider struct than yours, or a
compiler temporary.

A slot the client increments and never reads is a counter in the source; VC6 keeps the load,
`inc` and store. Declare it, and zero and increment it where the client does. Example:
`ColorPalette::CreateFromDC`.

A frame longer than its used slots is an aggregate with dead members: a `Vec3` whose `z` is never
written still reserves twelve bytes.

Read a type off the frame size. A surplus word can be a wider header struct, such as
`MCI_WAVE_OPEN_PARMS` against `MCI_OPEN_PARMS`. VC6's headers stop at DirectX 3, so declare a
later struct, such as the DirectX 5 `DDCAPS`, yourself.

A buffer one byte above a four-byte boundary (`lea` of `[esp+0xd]`) is the second byte of a
larger record whose first byte the format reserves. `NetSetupDialog::SelectPlayerPosition` reads
its name into `entry.szName` of a `PlayerSlotName`.

`and esp, 0xfffffff8` is ordinary C: VC6 realigns the stack when a `double` local is spilled
across calls. A `__cdecl` function with no `sub esp` can still have locals in dead parameter slots.

### Slot reuse and scope

VC6 overlays escaping locals only if their scopes are disjoint, so braces fix a frame too big by
one local's size, and scoping vectors into or out of blocks controls overlay both ways.

Slots the client gives to the setup's vectors and then to each arm's own are the setup in an
inline helper, whose locals end with its expansion. Example:
`Object::ComputeSteeringDirection`'s `ToObjectFrame`.

A named local at function scope cannot take a dead local's or a dead parameter's slot. Declared
inside the arm that reads it, it can. A four-byte record member can take a recycled parameter slot
too. A local or record of five bytes or more cannot: it goes to the aggregate pool and moves the
frame. Eight-byte temporaries never take a recycled parameter slot.

A block local shares a dead parameter's slot with a compiler temporary only from a disjoint arm.
Write the code that makes the temporary in the `if` arm and declare the local in the `else` arm,
even where the `if` arm returns. Declared after the `if`, one of them takes a frame slot. Example:
`AnimScriptTable::ParseScriptLine`.

A value the client keeps in a parameter's home slot is written back to the parameter in the
source; Ghidra shows it as a reassignment. Reassign the parameter rather than add a local.

A parameter's slot read on one path, where the tree copies the parameter, is an unset local that
VC6 homes on a parameter's slot. Leave the assignment out, then read the listing's `_name$ = N`
lines: where VC6 homes it on another parameter's slot than the client, the program reads a
different value, so refuse the change. Example: `MsgInteractor::Dispatch`.

Declaration order does not decide which dead parameter slot a block local takes. Where two block
locals land in each other's slots, the row is a frame-order wall: see [Known walls](#known-walls).
Two out-parameter `lea`s at different displacements are two locals, however dead they look.

A `lea` of a parameter's own slot passed as a buffer is a small local that took the slot once the
parameter went to a register: declare `char ch;` and pass `&ch`, don't pun the parameter.
Example: `PlanVM::ReadAll`. Each discarded by-value return gets its own temporary.

### Slot order at /O2

Declaration order does not order an `/O2` frame. The allocator ranks each free local:

- a scalar by the order its live range ends;
- an aggregate by its plain reference count, which loops and branches do not weight; on a tie the
  one referenced first sits lower;
- an array by how often the source names it in emitted code, ties broken by first reference. A
  reference VC6 does not emit, such as `(void)szCaption;`, does not count.

Scalars sit ahead of every aggregate, and same-size locals pack together. Reversing the assignment
order of scalars changes which ones spill, not the order of the slots they take. An aggregate of
eight bytes or less sits below an eight-byte compiler temporary; a larger one sits above it.

An aggregate's count is of the references left in the emitted code, after VC6 forwards stores and
keeps values on the x87 stack: the most referenced sits lowest. A statement that writes a result
into one of three vectors can move all three, and so can a change that stops a forward. Count the
`_name$` operands in a probe listing.

To count the client's side, track the stack depth through each `push`, `pop`, `sub esp` and
callee-cleaned call in its listing, and total the references per frame offset. Equal totals with
different per-offset counts mean references moved from one local to another. The rule is not
absolute: in `CollisionCheck::DromerRandomizerCollision` the client places a vector with fifteen
references above one with six.

A member of a record has no rank. It takes the record's own offset, so a local struct holding the
contested locals in the client's order would fix the map. Don't write one: a struct whose only
job is to order frame slots is banned at any cost. Group locals in a struct only where they are
one object of a type the tree has: a `Vec3`, a box, an array, a file or packet record.

Read a `perm` from `python tools/slot_residue.py` as a slot order the allocator chose. Where a
typed spelling does not move it, record the row as `frame-order` in `symbols/retired.csv`.

A local that holds two values can cost its neighbours their slots. Give each value its own local:
in `PlanCond::BallGotBehindMe`, reusing the Rok's position for the actor-to-spawn delta costs
twenty-four points. It can also turn an integer sum round (`GDI::ReadBitmapFile`).

Where every reference count matches, the tie within a group is not reachable by names,
declaration order or statement order. Outside such a tie, a local's name can move its slot:
retyping two `int` locals of `D3DRend::DrawFace` as `const TexCoord*` moved both slots with
identical instructions, and the old names under the new type put them back. Try a few names
before you record a cost.

Some rows follow the declarations their file sees. See
[Header-sensitive rows](#header-sensitive-rows) before you bisect a loss in an unrelated function.

A struct return is built above the named locals when it is twelve bytes, and low when it is
larger. Assigning the call result into a named local, separate from the declaration, builds the
value in place with no temporary:

```cpp
Point pt;
pt = GetOrigin();
```

A callee that copies a record through a pointer parameter and returns that pointer returns the
record by value. Its own bytes are the same either way, but each caller's out-parameter local is a
frame slot the client does not have: `tok = LookupSymbol(szToken);` removes them from
`PlanVM::EmitArgToken`.

A struct-returning call written in an argument list makes temporaries early. VC6 creates them
during semantic analysis, ahead of every inline expansion in the function. Where these push the
temporary map down, build the value inside the helper and pass pointers.

### The /Od frame

An `/Od` frame is laid out in five pools, from `ebp` down:

1. function-scope names, ordered by the name hash;
2. each block scope in turn, each ordered by the hash;
3. the `$T` temporaries and the inline-expansion groups, in expansion order, each group with its
   own locals first (hashed) and its argument temporaries last;
4. `this`;
5. code generation temporaries: `?:` results and `switch` values.

```python
v = 0
for c in name:
    v = ((v << 2) + (v >> 4) + ord(c)) & 0xffffffff
bucket = (v ^ (v >> 16)) & 15      # the name's bucket
```

Ascending buckets sit nearest `ebp`, and the later declaration wins a tie. Pick names whose
buckets rise along the client's displacements. `python tools/frame_search.py --buckets NAME...`
prints the buckets. With `--source` and `--mangled` naming a function, `--show` prints its map and
`--target` scores the current names against the client's map. Probe a naming with
`tools/probe_asm.py`.

A slot after the temporaries is an inlined helper's local; write the helper as a `static inline`
function in the file. A member call that returns a vector costs one temporary where a free
function costs two. A `switch` on a flag and a `?:` clamp give the pool 5 temporaries. Example:
`CollisionCage::ComputeFaceNormals`.

`/Od` differs from `/O2` in these ways:

- parentheses round a leaf emit nothing, and source order is emitted as written;
- `char* const p = (char*)this;` costs a slot;
- a dummy `edx` parameter costs four bytes of frame, so write a member;
- `delete p` on a typed pointer makes a temporary slot where `operator delete(p)` does not;
- `if (0)` is not folded;
- `eax`, `ecx` and `edx` rotate across statements without resetting at labels. An unreachable
  expression statement advances the rotation by one;
- `/Od` emits no COMDATs, so `/Od` bodies never fold at link.

`#pragma optimize("", off)` is close to `/Od`, not identical. Under the pragma the order of first
assignment also moves slots, and `__forceinline` still expands. Use the per-file flag where the
whole unit was `/Od`.

## Inlining and pragmas

Under `/Ob1` only a function marked `inline` or defined in its class body inlines; any other
function is always a call. The original's startup table has a thunk for every dynamic
initialiser, which VC6 emits under `/Ob1` and inlines away under `/Ob2`. The symptom of a wrong
keyword is a run of added instructions where the client has a `call`, or the reverse.

Where the keyword goes decides whether it works. A class member inlines when `inline` is on its
definition outside the class, even below the caller. A namespace function inlines only when its
first declaration is `inline`: a later `inline` on the definition alone is ignored. Write it on the
declaration in the header or in the file's namespace block.

An inline function keeps its out-of-line copy only where something needs it: a vtable slot, an
address taken, or a call VC6 did not expand. The copy is a public COMDAT, so callers in other files
that see only a plain declaration link to it. A virtual function the client inlines in its own
class's constructor, such as `InfoBar::HideMessage`, is `inline` on its definition and keeps its
`FUNCTION` row through the vtable.

Where the client calls a function that this build inlines, take the keyword off or move the body
out of the class. `#pragma auto_inline(off)` does nothing under `/Ob1`.
`python tools/find_inlined_calls.py` lists every call the client makes that this build inlines,
with the callee to take the keyword off. Apply one file's callees per build and keep only what
loses nothing.

Where the client inlines a wrapper in one file and calls it from others, the original called the
API directly in that file: `GDI::SafeDeleteBitmap` calls `::DeleteObject`.

Where a header inline must not expand in one caller, wrap the caller in `#pragma inline_depth(0)`
and `#pragma inline_depth()`.

A base destructor inlined into a derived destructor was a header inline. The client's derived
destructor ends in the base's vptr store with no call. Make the base destructor a header inline
with its `FUNCTION` marker above it: the scalar deleting destructor always calls the out-of-line
copy, so that row stays exact. Measure the whole tree.

### The inline budget

VC6 spends its inline budget per call site, and a large function's budget scales with its own
code.

`__forceinline` is for the inline budget only. Where VC6 stops expanding a plain `inline` function
late in a long caller, `__forceinline` keeps every site inlined; elsewhere plain `inline` gives the
same bytes. Past its budget, a base constructor inlined at the first dozen `new` sites becomes a
call, and each call brings an EH frame. Example: the `BehaviorOwner` constructor in
`GameEngine::CreateAgendaItemPools`.

A constructor that the client both inlines and calls cannot be reproduced by one spelling. A
second constructor chosen by a tag argument in the member initialiser list can give both, while
the destructor, vtable and out-of-line copy stay one symbol each.

First-level expansions spend the budget. Where the client inlines each member constructor of a
large class but calls the base constructor inside it, the first-level expansions used it up.
Turning a group of the body's own statements into a call to an inline helper moves the cut
earlier; a hand-expanded helper moves it later. Adding inline calls without removing statements
moves nothing. Example: `InfoBar::InfoBar`. A large function that inlines a helper in three
blocks and calls it in the fourth is the same wall.

The budget cuts nested expansions first: a base constructor or member `operator=` inside an
inlined constructor becomes a call a site or two before the outer constructor does. A client that
inlines a whole chain at one site and calls the outer function at the next has a cut no amount of
inline code above it reproduces (`RampObject::InitCollisionPlanes`). A one-line getter still
inlines late in a long caller.

### Optimisation regions and moved bodies

`#pragma optimize("", off)` and `("", on)` around a body reproduce an unoptimised body inside an
`/O2` unit. Before you insert a pair, check whether the insertion point is already inside an off
region. An unconditional `on` ends the region early and optimises its neighbours.

A body in the wrong optimisation region is a whole-function difference. Where only one side opens
with `push ebp` / `mov ebp, esp`, the body sits on the wrong side of an `optimize` pragma or a
per-file flag. `python tools/find_unoptimised.py` lists the client's unoptimised bodies.

A moved body keeps its pragmas only if the mover carries them. `tools/movefn.py` carries a pair
that opens above the body and closes below it, and refuses a body whose region covers a neighbour
or a target point under other pragmas. `python tools/lost_pragmas.py` checks git history for a body
that a move put under different pragmas, so it needs the commits around the move.

After merging or moving bodies, measure each fallen row. A `call` line that only the client has
names the callee VC6 now inlines: take the keyword off it, or move it out. If one body loses many
calls, move that body out rather than its helpers. Whichever side moves, the existing callers in
the other file decide.

A moved body can fall without anything inlining. Where a fallen row differs only in merged tails
or branch offsets, move the body above the unit's other bodies and measure again:
`String::FormatAlloc` matches only as the first body in `string.cpp`. `tools/movefn.py` inserts by
address, so a later move into the same file can undo such a placement.

### Other pragmas

`#pragma optimize("s", on)` gives `/O1` code for one unit; stock CRT source compiled that way
matches the library. `#pragma optimize("g", off)` stops constant commoning and costs the frame
pointer; it reproduces no client function here.

`#pragma function(strlen)` turns off an intrinsic for a unit where the client calls the library
routine; it costs where the client inlines. `#pragma intrinsic(memcpy)` does the reverse, for
example in an unoptimised region.

### Library inlines

- two `repne scasb` loops are `strlen` then `strcpy`;
- `rep stosd` then `stosw` or `stosb` is `memset`, with the size readable off the counts;
- `repe cmpsd` is `memcmp`;
- `strcmp` inlines at `/O2`, so a called compare is `_mbscmp` or another routine;
- `??2` and `??3` are `operator new` and `operator delete`, and `??_U` is array new;
- `xor reg, reg` then `sbb reg, reg` is VC6's boolean to mask.

VC6's inline `memset` loads the fill word before it computes the destination. Where the client
computes the destination first, write a byte loop, which emits the same string instructions.

Store or name an inlined `strcpy`'s answer where the client keeps it. Where the client interleaves
the next statement's loads with `rep movsb` and stores the member last, write
`m_pszName = strcpy(new char[strlen(pszName) + 1], pszName);`. Example:
`AnimNamedRef::AnimNamedRef`. `p = strcpy(p, src);` keeps the block in `edx` across the copy
where `strcpy(p, src);` does not. Array `new` in the one-line copy makes VC6 recompute the source
after the call, where `(char*)::operator new(...)` holds it in `esi`.

A `push ecx` frame slot nothing reads, around an inlined `new` and `strcpy`, is a helper that
takes the pointer by reference. Where the client also moves the copy's destination into `eax`
before `rep movsd`, the helper returns the copy: `pText = new; return strcpy(pText, src);`, as
`Str::Duplicate` does. Example: `TeamsBlock::ComputeTeamNameChecksum`.

Inline assembly is rare. An `/O2` function with a frame pointer is a suspect, and most are
ordinary C. It is assembly only when it also has an idiom from the list in
[Hand-written assembly](ANNOTATIONS.md#hand-written-assembly), no `and esp, -8`, no `fs:[0]` or
`_chkstk`, and it is not in an `/Od` unit.

## Switch tables

A jump table is a map of the source's case order, since VC6 emits case bodies in source order.
Rank each table entry's target by address on both sides, and write the cases in the order read
off the client's table. Compare only the common prefix, since reccmp cuts the original's listing.

Case labels shape the table:

- labels stacked on one body fold to a range check;
- four separate empty cases that each `goto` one label build a jump table with one address in four
  entries;
- an end case that does nothing needs its own body identical to the default;
- one `break` per case keeps the distinct-block count, which decides between a wide jump table and
  a byte index table.

A sparse switch emits a short dword table and a byte index table after the function, and reccmp's
row size includes them. `movzx ecx, byte ptr [idx]` then `jmp [ecx*4+tbl]` is a two-level switch.
Marking the adjacent data makes a `jmp [eax*4+OFFSET]` operand resolve.

`sub eax, N` / `dec eax` chains are a `switch`, not an `==` chain. An `if` chain never lowers to a
subtract chain. At `/O1` VC6 emits a subtract chain where `/O2` builds tables, so a mismatch there
is a flag question.

A switch on a signed `int` emits a signed range test. Write `switch ((unsigned int)x)` for `cmp` /
`ja`. A switch on a `short` promotes it with `movsx`.

The index takes the first register free at the switch head. `switch ((unsigned char)nId)` gives
`edx` where other spellings give `ecx`, because the mask leaves the unnarrowed value live in
`ecx`.

Use `LOWORD` and `HIWORD` for a message word, not `& 0xffff` and `>> 16`. The mask makes the
subject unsigned, and `(x >> 16) == 1` is peepholed to an `and` / `cmp`.

Cases that jump into the middle of an earlier case were written out in full; VC6 merges a case
body into an earlier case whose tail is the same statements. Write the shared block, such as
`ClearBlack(); InstantiateState(); break;`, in each case. A `break` to a shared block after the
switch puts the block after the last case. Example: `Game::UpdateState`.

A shared tail below a switch can cost the table order, and writing it out in each case can put
the order back. Per-case `return` lets VC6 fold a constant id; a `break` duplicates the tail after
propagation. Measure both.

A switch whose arms each copy their own string is a real shape. The tell is the client repeating
one `lea dest` in every arm.

A case that subscripts by the switch subject less the first label, with the bias folded into the
displacement (`[base + key*4 - 0xc4]`), has an `unsigned int` subject. With an `int` key VC6
commons `key - '1'` with the switch's normalisation. Example: `GameApp::HandleMatchKeypress`.

Cases that each load a constant index, with one `[reg*4 + table]` load after the join, index a
real array. Where the table is a record of named pointers, the record is an array of the common
base type with a typed accessor per entry. Example: `Network::ReceiveAssetPacket`.

A typed accessor that stands in for an indexed field read takes only the cases callers pass. Eight
cases build a jump table and nine bodies; three build a `sub` / `dec` chain. Where the client
reads `[record + index*4]` for every case, give the record a union with a `float` array view
instead, as `ClipVertex` has.

A mask loop that steps two pointers by 4 and reads a third through their difference walks three
float arrays: `while (nMask != 0)` over `ClipVertex::af` with three float pointers stepped
together. An indexed `for` gives one induction and two differences. Example:
`SWRend::ClipPolyAgainstPlane`.

An `add reg, 4` between two field tests is `*++pflAxis` over the record's `float` array. Example:
`Face::ClipCodeAndCullCheck`.

## Casts, widths and signedness

Read width and signedness off the instruction. `xor eax, eax` then `mov ax, [f]` is an
`unsigned short`. `movsx` is a `short`. `cmp dx, word ptr` is a short counter. `jb`, `ja` and `jbe`
are unsigned; `jl`, `jg` and `jle` are signed. `shr` is unsigned; `sar` is signed. A `movsx` at a
use site is only promotion and proves nothing about the variable.

A cast's width is part of its meaning. `*(AnimPlayerBlock*)(p + X)` is a block copy, not a short
read. Replace a cast with a field only where the widths match.

### Signedness

A pointer walk with a `jb` exit can be an `unsigned int` index, strength-reduced; an `int` index
gives `jl`. Example: `DirectPlay::GetErrorString`. Pointers compare unsigned, so `jl` against a
table end is an `int` index, or a `short` one where the client sign-extends it.

A subscript takes the element type's signedness. With `p` an `int*`, `p[1] >> 16` emits `sar`;
with `p` an `unsigned int*` it emits `shr`. Where the client shifts, divides or compares the word
unsigned, declare the element unsigned.

`0 < (unsigned int)(caps & 2)` emits `neg` / `sbb`. A single-bit test written `!= 0` emits `shr` /
`and 1`. `setg` is the signed form. Signed `x % 2` is `and reg, 0x80000001` with a
`dec` / `or` / `inc` fixup under `jns`; `& 1` is one `and`.

`mov reg, 0`, `sets`, `dec`, `and` is a conditional expression. VC6 emits it for
`n < 0 ? 0 : n`. An `if` gives a branch, and the mask written out as `((n < 0) - 1) & n` computes in
the other registers. Example: `PaletteScalarFade::ApplyPhase`.

### Narrow types

`cmp word ptr [mem], di` compares two values of one sixteen-bit type. A `short` against an
`unsigned short` promotes both sides, and the compare becomes `movsx` against `xor` and `mov` and
a dword `cmp`. Fix the field's declaration, not the site. Example: `AnimClipEntry::m_wSegment`.

A byte read through `unsigned char*` gives `xor edx, edx` / `mov dl`. Through `char*` with a cast,
it gives `and reg, 0xff`.

Dword stores to a slot whose every use is sixteen-bit are a spilled `short`. VC6 stores whole
registers into shorts and reads back sixteen bits.

A copy of a `short` that skips the `movsx` goes to a `short`. An `int` takes the extended copy
and moves the registers round it. Example: `SoundManager::Load`.

A `short` local pushed as a plain dword goes to a `short` parameter, and so on down the chain:
every parameter it passes through without a `movsx` is a `short`. Changing only the first callee
moves the `movsx` into it. Example: `AnimScriptTable::ParseScriptLine`'s trigger id.

A `short` getter the client compares as `movsx` against a 32-bit `cmp` is read into an `int`
local. A `short` parameter fed from it still takes the low word. Example:
`PlanCond::MeOrTeammateClosestToRok`.

A narrow parameter shows as a partial load then a whole push, `mov dl, [..]` / `push edx`. The
callee declares `char` or `short`.

Widen byte arithmetic through a named `int` to get `add edx, eax` then `mov [..], dl`.

### Masks and shifts

A narrowing cast between a shift and a mask stops VC6 folding the mask:

```cpp
(short)(x >> 24) & 0xff                // keeps shr and and
((unsigned char)(nFlags >> 4)) & 1     // shr al, 4 / test al, 1
((unsigned char)nFlags >> 4) & 1       // folds to test al, 0x10
```

The same holds for a compare: `(unsigned short)(nArg >> 16) == 0xfedc` keeps `shr eax, 0x10`,
where `nArg >> 16 == 0xfedc` folds to `and eax, 0xffff0000` / `cmp`. The cast needs a load wider
than itself; a field already read as a byte has nothing to narrow.

A mask that feeds a test takes the client's sixteen-bit `and` when the test stores or returns
constants, so change the consumer, not the mask. `if ((nA & 3) == (nB & 3)) { return 0; }` then
`return 1;` gives `and ax, 3`; returning `a != b`, or storing `(short)(a >= b)`, keeps thirty-two
bits. The client still gets `setcc` or `neg` / `sbb`, so the `if` is not a branch in the output.
Example: `GameEngine::IsComputerTeam`.

## Packed words and bit-fields

None of the client's packed words is a C bit-field. Probes show four codegen facts:

- A bit-field store masks every field to its width and drops the mask on the top field. A client
  that ORs an unmasked field below the top (`MsgEvent::Pack`'s middle octets) or masks the top
  field packed by hand.
- A bit-field compared with a constant is masked in place: `and reg, 0xffff0000` then
  `cmp reg, 0xfedc0000`. A hand test is `shr reg, 16` then `cmp reg, 0xfedc`. A struct of two
  shorts reads the high half with a word load, which the client's dword load and shift refute.
- A bit-field read from memory reloads the word in each block. A client that loads a byte once and
  tests it across branches (`MoveCodec::Decode`'s flags at 0x0042bef0) held a local copy.
- A bit-field read is the hand read, `shr` then `and`, so the unpack side never decides.

The typed form that reproduces the hand code is a record of one word with inline const getters,
copied into a `const` local: VC6 holds a one-member record in a register like the bare word. These
details decide it, each with what the wrong spelling cost:

- A sign or flag getter returns the masked bits, tested `!= 0` at the call. A `bool` getter adds a
  `setcc` or a shift.
- An index getter returns `int`. Returning `unsigned short` let VC6 common the read with the
  fall-through return, 100.00 to 53.85.
- A half the client loads with a word read (`xor edx, edx` then `mov dx, word ptr [eax+4]`) is a
  union field, not a getter. An inlined `PlanWord::GetPronoun` dropped five exact `PlanAction`
  rows as far as 41.03; the `wPronoun` field keeps all 26 sites exact or level.
- A word the client reloads for each field is not copied into a record local. Static getters that
  take the word at each call keep `GameEngine::ProcessInputCommandQueue` exact (0x004871c2); a
  `const` record copy cost 3.24.
- A mask the client keeps on a field that cannot need it shows the field read as a narrower type
  first: `NetSequenceWord::GetTarget` is `(short)(nWord >> 16) & 0xff`. Without the `short`,
  `EngineObject::ProcessReceivedPacket` fell to 92.43.
- A getter over a byte array takes the field's own signedness: `SlotNibbles::Get` on
  `const char*` keeps the client's `movsx`.
- An SDK macro that masks its halves is refuted by a client that does not: `MAKELPARAM` cost
  `Display::Initialize` 5.70.
- A record of one byte spills to the frame. Widen the byte into an `unsigned int` record, as
  `MoveScaleFlags` does.
- A word the client updates in place in each arm of a switch takes a static member over the
  pointer, which reads the word and stores into it per arm (`DecisionLog::Push` and `Pop` in
  `Interactor::ExecuteScript`). Taking the word by value and returning the new word merged the
  arms' stores and cost 29.91.

A static packer is free while its arguments are locals (`MovePositionWord::Set`). An argument that
is a call result becomes a parameter temporary, and VC6 then makes the next call before the OR the
client does first: `SystemSyncWord::Pack` costs `RokObject::Postpass` 10.29 points this way.

## Addressing and `lea`

VC6 folds `mov rD, rS` and `add rD, K` into `lea rD, [rS+K]` when the base is still live. This is
sufficient, not necessary: the client also folds with a dead base. Don't close a row on the rule
alone.

A pointer local folds its sum into the first read. Where the client adds `base + index` into one
register and then reads `[reg]`, write the subscript `m_pszText[m_nCursor]` at every use instead
of a `pAt` local. Example: `PlanVM::IsCommentOrGroup`.

### Mid-struct bases

A named pointer to a mid-struct object forces the shared `lea`. For member stores that share a
base with a call, `/O2` addresses every store off the object unless a name such as
`Label* pLabel = &m_label;` forces the base into a register. Three separate stores into a
mid-struct destination need the base as a named local too, `Vec3* const pScale = &p->m_vScale;`.

For calls on a mid-struct member, write the member spelling at every site and let VC6 common the
`lea`. With the object in a callee-saved register across both calls, either spelling commons it.

Assign a member's address to a local first where the call needs `lea ecx, [eax+K]`. Put a call's
result in a local before masking it or adding an offset.

Where the client forms a fastcall argument with `lea edx, [eax+K]` straight after a call, name the
object the call returns, `pBody = pRok->GetPhysicsBody();`, then pass `&pBody->vel`. A named
`Vec3*` gives `add eax, 0x20` and `mov edx, eax`. Example: `PlanAction::Launch`.

Derive a mid-struct base straight from the global, not through a local.

Where the client holds a field's address in a callee-saved register across a call, name the
pointer, and assign it just before its use. Where the client folds the displacement into each
access, don't name it.

A struct assignment creates a base pointer. `p->m_vScale = v` forces `lea eax, [edi+0x44]` and
three stores; three component stores remove it. Use one struct assignment where the client copies
through a base, and component stores where it does not. A twelve-byte copy out of a returned
pointer, `v = *f(); *pOut = v;`, batches three loads then three stores.

### The rebase

The mid-struct rebase has three behaviours. With one use of the base pointer, VC6 folds the rebase
away; with two, it materialises it before the first use. For a read-modify-write of one component,
statement adjacency decides:

```cpp
p = base + off; fl = *p; *p = fl + x;     // add eax, off before the read
p = base + off; *p = *p + x;              // fadd [eax+off], then add eax, off
p = base + off; fl = x; *p = *p + fl;     // the pointer is deleted
```

The rebase lands after the read only when the read-modify-write is the statement right after the
pointer's assignment. Apply the advance to the variable a global store used:
`p = table[SLOT]; g_pThing = p; p += OFF;` behaves differently from
`char* p = g_pThing; p += OFF;`, because VC6 forwards the stored value.

A rebase between the second and third read of a vector is an inline helper's pointer parameter.
Where the client reads `[eax+0x34]` and `[eax+0x30]`, then does `add eax, 0x2c` and reads `[eax]`,
call `Vec3::DotInline` with the row from a named body pointer. A parenthesised leaf, or a `char*`
advanced to the row, rebases before the first read. Where two call sites differ in the x term's
operand order, give each site's pointers their own names. Example: `PlanCond::TakeDamage`.

An `add eax, imm` between a `fadd` and its `fstp` is the same tell on an accumulation: write it as
an inline member of the class that takes the vector by pointer. A static helper that also takes
the destination by pointer gets the rebase, but keeps the next statement's loads below its
stores. Example: `CProjectAndBounce::Release`.

A written-out dot product or squared length compiles the same as `Vec3::DotInline` at most sites.
Call the helper, and measure: a few sites keep a written-out sum because it scores higher, and
`Vec3::Magnitude()` against `(float)sqrt(Vec3::DotInline(&v, &v))` differs by site.

A sum loop's walker can point at the middle field to reach the client's biased base. A walker with
displacements that straddle zero is strength reduction: write the subscript and let VC6 make the
cursor.

## Constructors, destructors, EH and trylevels

`/GX` gives every function with a live destructor an exception frame: `push -1`, a handler,
`fs:[0]`, and a trylevel slot the function updates as objects finish constructing. The trylevel
stores are the best evidence of what the source constructs.

### Construction order

Constructor order is base, then members with constructors, then the vptr store, then the body. A
loop that the client runs before its vptr store is a member's constructor, such as
`ParticleSystem`'s `ParticleRow() { ... }`; a body loop can never land there.

The member initialiser list sinks the vptr store below the member stores. The vptr store depends
on nothing, so VC6 places it freely: among members assigned in the body, below members listed,
which is where the client usually has it. The sink follows the derived constructor's list. A class
may list only its own members (C2614). Some classes want exactly one member listed; measure.

A store the client makes ahead of the initialiser list's stores and the vptr is a member with a
constructor of its own, which runs first. Where an inlined constructor clears a head word first,
declare that word as a small struct whose constructor clears it. Example:
`Game::InitializeEngine`, for `AnimScriptTable`.

A store the client makes before a member's constructor call is a member initialiser: an orig-only
store to a low member offset above a `call`, and the same store recomp-only below it.

VC6 emits the vptr store itself, and a hand-written store cannot reproduce it. Derive from a
polymorphic base, put the `VTABLE` marker on the class, and delete any hand-written store. VC6
drops a base's vptr store that an inlined derived constructor overwrites. A derived class that
declares no virtual of its own gets no vptr store. `__declspec(novtable)` also suppresses the
class's own store.

A constructor store after an early return means the source declares the object after that guard,
just before its first use. Two objects that each start just before their own call are two
declarations. Example: `GameApp::CopyCDFile`.

An explicit constructor call is placement new without the null test. `g_str.String::String(s)`
constructs in place. It also stops VC6 inlining that constructor.

A local of a type with a constructor emits a call the client may not make. Where the client
constructs nothing, the local is a plain struct.

A POD returned in memory rather than `edx:eax` has a constructor. An empty `Point() {}` is enough.
An eight-byte pair returned by value needs its empty destructor too, or it comes back in
`edx:eax`.

A class passed by value is copy-constructed into an argument slot and destroyed by the callee.
Return by value uses a hidden pointer that the callee pops, and `return temp;` needs a real copy
constructor.

### Unwind states

A trylevel the client raises that this build does not is a member with a destructor. VC6 raises
the state once per constructed member whose type has a destructor, even an empty one. Wrap plain
aggregate members to give them one, `struct RadarRect : RECT { ~RadarRect() {} };`, above the
class's `VTABLE` marker. A constructor diff that is only the trylevel, or a destructor with an
unexplained register swap, is this. Example: `Radar::Radar`.

A local whose state goes to 0 before `String::Reserve` and to 1 before its next call is a
`PathString`, `PathString strPath(n);`, with the next call its own statement. Example:
`AudioMgr::PreloadGroup`.

Byte unwind states with no -1 mean an object with a destructor is live from the top; an empty
destructor on a header struct gives it. Unwind states 0, 1, 2 written as bytes mean an object with
a destructor constructed at a by-value call's return: declare it after the call.

An exception frame with no `try` is a `new T` expression where `T` has a constructor: the size
push, the pointer store, the unwind state, the null test, and a `jmp` over an `xor`.
`operator new` followed by an init call has no frame. A `__fastcall f(T*)` that returns its
argument is a constructor. `T` must have the right `sizeof`.

A constructor's exception frame can come from its base's destructor: an empty `~Base()` gives
`push -1` / `fs:[0]`, and a destructor on the derived class alone does not. An intermediate class
between a destructible base and its derived classes costs an unwind funclet in every `new`.

Declare quiet destructors `throw()`. VC6 emits a state update before any call that can unwind, and
treats a destructor as able to unwind unless it is `throw()`; the tell is an entry state off by one
and an extra state store. `extern "C"` callees are assumed not to throw. Where the client skips
the -1 store between a `new T[n]` and the next call, that callee is `throw()`
(`JoystickDevice::Poll`).

A counting unwind state is separate members, not an array. An array member pushes the element
size and calls the vector constructor iterator, `??_L@YGXPAXIHP6EX0@Z1@Z`, which needs a `LIBRARY`
marker under its mangled name.

Try and catch blocks are recovered from the FuncInfo record: its try block count and the catch
type descriptors.

### Destructors and deleters

`test esi, esi` / `je` / `push 1` / `call [eax]` is a `delete` through a virtual destructor in slot
zero. Where this build inlines a destructor and calls `operator delete`, a derived object skips
its own destructor. Give the class a virtual destructor and write `delete p`. A root whose
original table is pure (`Renderer`) is `__declspec(novtable)`, so the derived destructors end on
their own vptr store. Example: `AgendaItem::Activate`'s `delete this`.

Only a root class's deleting destructor inlines its destructor. For a derived class VC6 SP3 calls
the destructor from the deleter at `/O1`, `/O2` and `/Ob2` alike, whether the destructor is in the
class, `inline` outside it, `virtual`, `__forceinline` or implicit. A client deleter with a bare
vptr store for a derived class is a wall, as in `BehaviorOwner`.

A derived deleter that calls its base's destructor means the class declared none. An implicit
destructor stores no vptr. With an out-of-line base destructor it is a `jmp` to it; with a header
inline one it is a copy of the base's body, which the link folds onto the base's. An empty
`~X() {}` stores `X`'s vptr and gives the deleter a body the client lacks.
`python tools/vtable_behaviour.py` lists the deleter slots. Example: `AwardsUIScreen`.

An explicit destructor call is a tail jump. `p->C::~C()` compiles to the scalar deleting destructor
inlined with flag 0; write it qualified. `&A::~A` is C2277, so where the client passes a destructor
as a function pointer, add a forwarder.

## Data, globals and link order

reccmp pairs a direct `[global]` operand by name, so a marked global at the wrong address costs
nothing there. Three cases compare raw addresses instead, and only the data layout reaches them:

- a scaled index, `[reg*4 + table]`;
- a loop bound that VC6 strength-reduced to the address one past an array. Each image names
  whatever object the linker put there;
- an interior address of an array whose PDB record has no bound, such as `stderr`, `&_iob[2]`.
  This one is a scoring fault: a size row in `symbols/library_data.csv` makes both sides render
  the element.

When one instruction names a different data symbol on each side and it is a bare `cmp reg, SYM`
or an indexed `[reg + SYM]`, the operand is a computed address. Fix the data order, not the loop.
`python tools/global_drift.py --indexed` lists the globals a scaled index reaches that sit at the
wrong address.

### Placing data

A tentative definition goes to COMMON, and the linker orders COMMON on its own. `int x;` is
tentative; `int x = 0;` is not. Declaration order in `src/globals.cpp` changes nothing until every
member of a run has an explicit initialiser. COMMON also groups by width. Renaming a tentative
global can reorder its block, so a rename that touches one is a layout change.

A table read through a one-based index pairs only beside its client neighbour, since the
displacement is the table less one element. Give both an explicit initialiser, side by side in
the client's order: `g_aStationaryPlans` directly before `g_aStationarySlots`.

VC6 pools a unit's string literals after all of its named data, so a table whose end bound is a
literal pairs only when it is the last thing the file declares; `const` moves the table without
moving the strings. Where the original side shows that
bound as a raw address, the table belongs to the unit that owns the literal: move its definition to
the end of that file and put a `STRING` marker above the literal. Example:
`GameEngine::SetDifficultyLevel`.

An empty literal lives in `.bss`, so a zeroed "global" there may be `""`. `/Gf` (in `/O1` too)
pools it across units, the IJG library's included: `jpeg_open_backing_store` and the game read
`g_strHudDefaultName` as `""`, not as a zeroed array.

One dynamic initialiser that stores several adjacent globals initialises one record, since VC6
emits one initialiser per declaration:
`Vec3 g_vViewSteepness = { g_flZeroSteering, g_flZeroSteering, g_flTwoSteering };`. Mark the
record at its first word and read the old globals as its fields.

A marked global whose address lies inside another marked object is that object's member. Two names
give two variables, and the second is never written. Delete the global and read the member: the
arena's radii at `0x004cf030` are `g_arenaEllipsoid` plus 0x38.

Bytes the original reaches through one address are one variable. `Face::ClipPolygon` takes the
face staging count's address and reads the mask at +4 and the vertices at +0xc. Three `GLOBAL`
markers each matched their own operands, so the score never showed the linker placing them apart,
and every clip wrote over the globals after the count. Declare such bytes as one struct under one
marker and `STATIC_ASSERT` its offsets. The same fault shows as a memset of N words from one
global, a loop that walks one global until the address of the next, or a second marked name inside
another global's span. `python tools/scattered_records.py` lists all four;
`symbols/scattered_records.csv` holds the rows read and refused.

Size a global by the object that lives there, not by the field one site reads. A global declared
one word wide and indexed as an array writes into its neighbours, and the score does not show it.
Give every extern array its bound; `tools/check_extern_bounds.py` fails on one without.

### Link order

Link order is part of the match. The order of `HYPERX_SOURCES` in `CMakeLists.txt` decides where
each unit's data lands, and which symbol follows a table. Append new sources at the end. Keep a
moved file at its old position; changing only its path is byte neutral, and re-sorting the list is
not. A body moved back out of a merged file goes back to its old link position.

The list is not in the original's order, and sorting it costs rows. Only 30 of 340 code-bearing
files sit in the client's relative order, and the tree cuts units differently from the client, so
no order makes both images agree. Sorted by each file's first marked function, the build lost 21
exact rows: the `$E` markers paired with other units' thunks. Code order alone costs nothing,
because reccmp pairs functions by line or name.

What the order must satisfy is the `$E` thunk order. VC6 numbers each unit's initialiser and atexit
thunks `$E1`, `$E2` and so on, so many objects define each name. reccmp pairs a `$En` marker with
the lowest unpaired `$En` of ours, both sides in address order. So the objects that own marked
thunks must link in the order of the client's thunk addresses, and an unmarked thunk of that name
must link after the last marked one. `symbols/link_order.csv` names each marked thunk's object with
the client's evidence, and `python tools/check_link_order.py` fails when `HYPERX_SOURCES` would pair
a marker with another object's thunk. A thunk that names a function-local static still does not
pair: the client's address of the static is an immediate, and our PDB gives the static no symbol.

To place a file, take its first marked function in the client and its thunk addresses, and put it
where both orders hold. The startup table at `0x004bb004` runs in client code order, so an
initialiser's address is also its unit's position.

A unit's `$E` names follow from its definitions, so predict them before a move. VC6 numbers each
dynamic initialiser in definition order: a plain one takes three numbers (body `$E(n+1)`, table
thunk `$E(n+2)`, then a data symbol), and an object with a destructor takes five (body, destructor
thunk, atexit registration, table thunk, data). Only the function names enter the pairing. Moving
a definition out of a file renumbers everything after it in that file, so rename its markers from
the new map. Moving one in gives the file `$E1`, `$E2` and the rest, and the file must then link
after every marked owner of those names, or carry a marker of its own placed in client order.
reccmp pairs the startup table's entries by the data they write, not by position, so neither move
costs an initialiser row.

A dynamic initialiser renumbers the function-local statics after it. VC6 draws the guard byte's
`$S` number and the static's `$E` thunks from the same counter. Three triples ahead of
`RokObject::ApplyRenderSlotPose` rename its guard `?$S1@` to `?$S10@` and its thunk `$E2` to
`$E11`. Fix the `symbols/static_guards.csv` row and the marker together.
`check_link_order.py` also fingerprints each marked pair by the matched entities its thunks
reference, so a marker that names another thunk of the right file fails. Two thunks that differ
only in unmatched data still look alike, so read the moved rows after every renumbering.

#### Which units sit where

A triple moves into its unit's file only where that file's link position fits the client's `$E2`
order. Every moved triple gives its file a `$E2` thunk, and other files already own marked `$E2`
thunks. A file linked before the last of them needs its own `$E2` marked, so its position among
those owners must match the client's address order. For example, `config.cpp` holds g_strPrefs's
unit and links straight after `globals.cpp`, because its `$E2` (`0x0046d530`) is the next marked
`$E2` after g_gameApp's (`0x0044c060`). Some thunks fit no order: the arena's destructor thunk
`0x0045c6d0` stays unmarked, since the client orders its `$E11` after `staticinitglobals.cpp`'s and
before `config.cpp`'s.

Link a walled unit's file after every marked owner of each name, and it can leave all its thunks
unmarked: reccmp pairs them through the startup table. `hud.cpp` links after `playerobject.cpp`
this way, though its code follows AnimPlayer's. Each new triple renumbers its file's guards, so
give `symbols/static_guards.csv` the new `$S` numbers or the guard rows drop.

A unit with destructor thunks still needs markers, because the startup table does not list them.
`dsoundmgr.cpp` and `soundchannel.cpp` link straight after `playerobject.cpp`, ahead of every
unmarked `$E2`, with their thunks marked. g_mixerWaveOut's `$E7` and `$E8` then follow three
unmarked triples in `config.cpp`, `meshbone.cpp` and `rokobject.cpp`, so those six thunks carry
markers. Mark only the names whose position fits; leave the rest
unmarked behind their last owner. `check_link_order.py` names a marker that lands on the wrong
thunk, including a body marked as its thunk.

### Unit boundaries

Without `/Gy` each unit's code is one run in link order, so a `.cpp` whose bodies form separate
runs holds several units. Three things confirm a boundary:

- A triple of `mov eax, [k]; mov [g], eax; ret` initialisers sharing one `k` heads every unit that
  includes the header behind them. The startup table at `0x004bb004` lists them in link order.
- A unit's `.data` is its named data, then its literals. Named data after another unit's literals
  starts a new unit, even with no triple.
- A table's one-past-the-end bound names the next object in the image, which is either the unit's
  first literal or the next unit's first datum.

Split a file at those starts and give each piece the original's link position. `planaction.cpp`
held four units and part of a fifth; the split was byte neutral inside every function. Linked
first, as in the original, the `"Human"` literal stays in its own unit (`/Gf` keeps the first copy
in link order) and `PlanAction::BodySplit` pairs. A table defined in `src/globals.cpp` belongs at
the end of its own unit's file: `CPlayer::SendPacket` needs `g_aSendFlagsByType` at the end of
`cplayer.cpp`, non-const, as its `.data` address shows.

The literal pool confirms a merge. A unit's literals follow its code order, so two files whose
bodies alternate and whose literals alternate in the same order are one unit.

Three things limit a data move:

- A moved global goes at its address among the unit's existing data, not at the end of the file,
  or it can take the place of a literal another table bounds on.
- A global defined in the unit that reads it can compile differently there. Defined in
  `collinteractorpool.cpp`, `short g_wCollInteractorPoolSize` cost `CollInteractorPool::Init` its
  match, so the original defined it elsewhere.
- Explicitly zeroed `.bss` keeps its unit's order only within one object. The key-config arrays sit
  between two other units' `.bss` in the original, and loops bound on both neighbours, so they stay
  in `src/globals.cpp` until the link order matches.

### Declarations and markers

Opening a new access section reorders a class layout in VC6, so check field offsets after any
change to a class body. An unbalanced `#pragma pack(push, 1)` packs every struct after it: when you
move a type out of a header, move its `pack` pragmas, `extern "C"` and `#ifdef` with it. An
allocation `push` is the class size, and a header that disagrees is wrong.

Write string literals inline, not through a `static const char*` or a local. A table of strings
must be an array. A `.rdata` pointer read once by a `mov` is a `const char* const`.

Write a function-local static as a real `static`; `/O2` folds a hand-written guard's constant. Its
guard byte, `?$S1@...`, has no declaration for a marker to sit on, so give the original's guard
address a row in `symbols/static_guards.csv`, read from the client's guard test, with our side's
mangled name in the `name` column. `check_markers_resolve.py` fails on a guard without one.

Write code and data addresses as `&Name` against a marker, not as integer literals, which stop
pairing once our image grows past them.

`call dword ptr [addr]`, or `mov edi, [slot]` then `call edi`, is an import thunk, not a global. A
`LIBRARY` marker pairs a thunk.

Name the C runtime by its linker names. Mark a CRT global under its undecorated source name, such
as `__piob`. Define every public symbol of a library object you write, or the linker pulls the
`LIBCMT` object as well. Stock VC6 SP3 CRT source compiles to the same bytes under the right
per-file flags.

Declare a GUID the client reads as `EXTERN_C const GUID IID_X;` under a `GLOBAL` marker, and link
`dxguid.lib`.

## Calling conventions

A caller shows the convention the callee cannot. Before you call a register choice an allocation
fault, read the callee's `ret N` and first instruction, and every caller's pushes and cleanup.

Count arguments from the stack. A `__cdecl` callee's arity is at most the caller's `add esp` over
four. The highest `[esp+N]` the callee reads gives a lower bound. `push 0` then `pop ecx` is an
argument and its cleanup. Decompilers drop register arguments, varargs and unused parameters, so
trust the pushes.

### `__thiscall` and members

`mov ecx, ...` before a call means `__thiscall`, even if the callee ignores `this`; a member that
never touches `this` compiles to the same bytes as a `__stdcall` free function.
`python tools/missing_this.py` lists rows where the client loads `ecx` and this build does not. A
function that passes its own `ecx` on to a callee is a member.

A fresh object in `ecx` before a call to a folded empty body is a member call on it. Write an
empty member of the object's class and call it on the object; the call's name still differs, and
the code around it pairs. Example: `Game::InitializeEngine`.

A handler that never writes `ecx`, even for a switch index, and calls a folded empty body with no
`mov ecx`, passes its own `this`. A widget handler's default arm is a qualified call to the base
slot, `Screen::OnWidgetMessage(nMsg, nId);`. Examples: the pause menus.

A folded body's marker names every call to it. reccmp prints the original's calls with the
marker's name, so put the marker on the body most callers in the tree name.

A variadic member is `__cdecl` and pushes `this` last, so one member can answer both a vtable slot
and a six-argument call.

### `__fastcall`

A `__fastcall` free function is not a member to the code generator. The ABI can be identical, but
as a `__fastcall` the `lea` of a local for a member call is commoned with an earlier one and
emitted before the pushed arguments; as a member it comes after them.

A dummy `edx` parameter costs an `xor edx, edx` at every call site, so
`__fastcall(void* pThis, int nEdxUnused, ...)` is right only where the client's callers zero
`edx`; otherwise convert it to a member. A callee that reads `edx` before touching the stack takes
a real second argument.

VC6 emits a `__fastcall`'s stack pushes before its register loads, whatever the spelling. A
parameter goes in a register only while it fits one, and the first that does not ends the register
list: `__fastcall(char*, float, float)` passes `ecx` and two floats on the stack.
`mov [esp+N], dl` early in a callee means `__fastcall`.

A callee's parameter order is a caller's schedule, since callers evaluate arguments right to left.
Where several callers set up one argument a slot late around calls to one function, change that
function's parameter order, with the declaration, definition and every call in one commit.

### `__stdcall`, `__cdecl` and COM

A function with no arguments emits the same `ret` and call site under `__stdcall` and `__cdecl`,
but VC6 schedules the body and the caller's deferred `add esp` flush differently. Before you
retire a zero-argument function on the scheduler wall, try the other convention. A function
stored as a `void (__stdcall*)(void)` cannot change.

A COM call pushes the object and cleans nothing. Declare the interface as a class of
`virtual ... __stdcall` members in slot order, which take the object on the stack, not in `ecx`,
or a `__stdcall` pointer table. `DECLARE_INTERFACE_`
declares the three `IUnknown` methods itself, so the Nth `STDMETHOD` is slot N. `__thiscall` on a
typedef is C4234: use an abstract class of pure virtuals in slot order.

Windows A and W macros rename members such as `LoadString`, `DrawText` or `MessageBox` inside
their own class. Rename the member.

## Types and records

A record's stride is evidence; `sizeof` is a claim. VC6 pads a struct to its widest member's
alignment, so a C struct with the right fields can have the wrong size. Put a size check beside
every decoded record, `STATIC_ASSERT(sizeof(MeshMaterial) == 0x16);`, with the stride its readers
walk. `#pragma pack(push, 2)` models a 22-byte record. Don't replace a literal copy length with
`sizeof` until the check passes.

A struct of an `int` and a `short` is eight bytes, so a struct assignment copies two dwords and
loses the word store. Write two typed assignments.

Retyping a pointer rescales every byte add on it. When a `char*` becomes a record pointer, each
surviving `p + OFFSET` scales by the record size, and a cast round it hides the change from the
compiler. Run `python tools/check_scaled_offsets.py`, which the battery includes, before you
believe a byte price for a retype.

A pun rewrite needs the declared type. `*(char***)&x` and `(char**)x` are the same value only when
`x` is a pointer. When `x` is an array, the first reads its first word.

Typing a pointer walked as an array can cost. A `char*` or `void*` walk lets VC6 assume nothing
about aliasing, so the loads stay put. A typed pointer lets it hoist and sink them, and no spelling
takes that back. Scalar field reads through the same typed pointer usually cost nothing.

### Unions

A float stored to the frame and read back as an integer or a half word is a union local.
`FloatBits` keeps the bits in a register. Where the client stores with `fst`, zeroes the slot in
memory and reads the word or its top half back from the slot, declare a union of the float, the
word and the halves. Example: `Anim::EulerToMatrix`'s `FloatWord`.

An `int` field written through a `short*` and read at both widths is a union member. Where the
client scans `%hd` straight into an `int` field and other readers compare its low word, give the
field a union with a `short` view and scan into that (`AnimNode::m_wLoopCount`). The same holds
for a byte the client stores or loads in a word field: `mov byte ptr [ebx+8], 0` is
`m_abChecksum[0] = 0` over a union with `m_nChecksum`. Keep the shifting getters where the client
shifts: `D3DRend::DrawFace` drops 15 points when they read bytes.

### Declarations as inputs

Measure a new accessor on its own. Naming a field is free; a new inline accessor can be the whole
cost of a change that looks like naming. Where a class already has an accessor, read the field
through it.

A member declaration nothing calls is still an input: see
[Header-sensitive rows](#header-sensitive-rows). Renaming a virtual can reorder VC6's tables, so
measure a method rename like a layout change.

`delete p` on an incomplete type only warns (C4150), and emits a plain `operator delete` in place
of the virtual destructor call. An incremental build shows warnings only for the units it
compiled, so after a header split or an include sweep, build clean and search for C4150.

An unqualified call inside a namespace member finds the namespace first: inside
`GDI::RealizePalette`, a bare `RealizePalette(` calls itself. Qualify the global with `::`.

A cast to a sibling class picks that sibling's non-virtual body of the same name. That changes
behaviour and the score cannot see it.

### Vtables

A vtable row's diff is a behaviour check. Read each slot where the two sides differ. Where ours
names a real client function other than the client's, the class inherits a body the original
overrides. Where ours is a stand-in for a folded body, the stand-in must do what the client's body
does. Both are faults the function scores do not show: `AwardsUIScreen` answered button 0x34 with
its base's handler.

`python tools/vtable_behaviour.py` separates folds from faults. It compares both bodies of every
slot where the names differ, with addresses blanked. A slot whose bodies match is a fold and needs
nothing. Each slot it lists runs other code than the original's.

A comment that leaves a slot to the base is a claim. Check that the base's body answers what the
original's slot answers: `CAvoidObjects` was left to a base that answers nought where the
original's stub answers one.

A stand-in takes the folded body's return type and code. A destroy stand-in declared `void` still
leaves the freed pointer in `eax`, where the folded body returns nought
(`PlayerBase::DestroyViaVtable`). Write the folded body's statements, not a call to another member.

A body that only one table holds is that class's override. Where Ghidra shows one slot of one
table as its only reference, put the marker on that class's override and delete any forwarder.
Example: `RokObject::Postpass`.

Where many vtables want one body in one slot, the missing class is a layer with no fields: an
intermediate class that overrides only that slot and changes no offsets. A derived class that declares no virtuals has a
table that copies its base's; where the original's slot names a function this tree already has,
declare that member `virtual` under the base's slot name.

A base whose original table is pure cannot share a body its derived tables fold. The three pool
owner bases hold pure `Destroy` slots, and sixty-two derived tables hold one `Destroy` body.
Abstract bases would need one `Destroy` for each family, and only one can carry the marker, so
the bases stay concrete.

A pure slot must resolve to the library's `_purecall`, not a local function with the same body.

`call dword ptr [vftable+N]` in a constructor is a virtual call inside an inlined member. VC6 turns
a virtual call written in a constructor into a direct call, but through a small inlined member it
folds the stored vptr into the load and calls through the table. Write that member call; don't
put a `GLOBAL` at the slot's address. Example: `PlayerObject::PlayerObject`.

## Known walls

These residue shapes have no known source spelling. Several walls have fallen to new levers, so
re-test a class when a lever lands that could reach it. Run `python tools/fault_classes.py` before
you pick a row, and take work from its `open` bucket.

### Link folding

The original linked `/OPT:REF`, which in VC6 also folds identical functions, so one address names
several bodies. `hyperx_sym` links `/OPT:NOREF` and keeps them apart, so an operand that names a
folded body reads as the wrong function. `/OPT:ICF` on our build folds our own identical bodies,
not the client's, and the folded functions lose their symbols. A `FOLDED` marker cannot restore a
discarded symbol. A repeated immediate where VC6 would common one value is the same wall: the
source named separate functions with one body, and the linker folded them.
`python tools/check_folded.py` must report no folded addresses in our build.

Write those functions anyway. Give each slot its own function, named for what the slot holds, and
write the copies the linker folded `inline` in the header, as `BehParam::SetKeyMonitorMode` is.
The marker goes on the copy the client kept, which sits where the first function to name it would.
Global optimisation then commons only a function the source really names twice, and the residue is
the folded operands alone. Example: `PlanVM::InitRuntimeOpcodeTable`.

The original folded only some identical bodies. Its vtables share bodies across unrelated classes:
`AgendaItem`'s slots 1, 2 and 12 all name the `ret 4` at `0x0040b910`. But the same image keeps six
`ret` bodies apart, among them `Object::Trace` and `Stub::Assert`, and five `xor eax, eax; ret`
bodies, which a full `/OPT:ICF` would have folded. The pattern fits objects built without `/Gy`:
`/O2` implies `/Gy`, which puts every function in its own COMDAT, while inline and compiler-made
functions are COMDATs either way, and only COMDATs fold. A VC6 probe object shows both halves. The
original's `.text` (0xaf12b) is near our `/OPT:REF` image's (0xaeb4d), not our `/OPT:NOREF`
image's (0xb499d).

Measured on `hyperx_sym`, against the `/OPT:NOREF` baseline:

- `/OPT:NOREF /OPT:ICF`: 122 exact rows lost, 325.6 weighted bytes, 183 rows down and 8 up. Our
  fold groups put `Object::Trace`, `Stub::Assert` and 27 other bodies on one body, and the
  survivor's lines belong to whichever function links first, so 29 rows lose their symbol.
- `/Ox /Gf` with the folded slot bodies inline, as the original's vtables show them: 18 exact lost,
  307.4 weighted bytes, 16 rows folded away and none up. Each fold group keeps the body that links
  first, so most markers land on a body that folds away.

Our `/OPT:ICF` also folds `__matherr` and the two libjpeg `ret` bodies, which the original keeps
apart. Those library rows alone cost 5 to 10 exact rows and 38 to 50 weighted bytes, more than
every call operand it could pair, so `/OPT:ICF` stays off. Our link order does not place functions
where the original's did, so moving markers cannot pick the same survivors.

### Why the game builds without `/Gy`

VC6 has no `/Gy-` or `/GF-`: `/O2 /Gy-` still makes a COMDAT of every function. `/Ox` is
`/Og /Oi /Ot /Oy /Ob1 /Gs`, so `/O2` without `/Gy` is `/Ox /Gf`. A probe object shows the string
halves. `/O2` and `/Ox /Gf` pool each literal as a writable COMDAT in `.data`. `/Ox /GF` pools it
read-only in `.rdata`. `/Ox` alone keeps every literal as its own `.data` static. The original
keeps 2010 strings in `.data`, no two the same, and its `.rdata` is 0x9522, near our 0x985a. So
the original pooled writable strings, which `/O2` and `/Ox /Gf` both do. Each variant, clean-built
and measured on `hyperx_sym` against `/O2`:

| Game flags | `hyperx_sym` link | Exact | Weighted bytes | Strings |
|---|---|---|---|---|
| `/Ox /Gf` | `/OPT:NOREF` | 0 | 0.0 | as `/O2` |
| `/Ox /Gf` | `/OPT:NOREF /OPT:ICF` | -12 | -89.3 | as `/O2` |
| `/Ox /GF` | `/OPT:NOREF` | -1 | -14.6 | in `.rdata` |
| `/Ox /GF` | `/OPT:NOREF /OPT:ICF` | -6 | -37.7 | in `.rdata` |
| `/Ox` | `/OPT:NOREF` | -292 | -19850.6 | 1089 extra copies |
| `/Ox` | `/OPT:NOREF /OPT:ICF` | -297 | -19873.6 | 1089 extra copies |
| `/Ox /Gy` | `/OPT:NOREF` | -292 | -19850.6 | 1089 extra copies |
| `/Ox /Gy` | `/OPT:NOREF /OPT:ICF` | -396 | -20176.2 | 1089 extra copies |

`/Ox /Gf` scores the same as `/O2` on every row, and it changes `HYPERX.exe` toward the original:
`Object::Trace` and `Stub::Assert` keep their own addresses, and `.text` is 0xaecdd, against
`/O2`'s 0xaeb4d and the original's 0xaf12b, because `/OPT:REF` cannot drop an unused function that
is not a COMDAT. Every `/OPT:ICF` row loses, so `hyperx_sym` stays `/OPT:NOREF` alone. The
`/Ox /Gf` ICF row counts the 8 rows that fold away as lost; the other ICF rows understate the loss.

### Other walls

- Scaled-index calls. reccmp compares `call dword ptr [reg*4 + table]` as a raw address, because it
  runs only one of its two operand substitutions on an indirect `call`; the same operand in a
  `mov` pairs. `tools/reccmp_call_table.patch` carries the one-line fix to reccmp.
- C runtime data. Operands that name the CRT's own unsymbolled data, such as the SEH scope tables,
  sit at addresses the linker chose, and moving one global does not fix it. A library literal
  takes a `string` or `widechar` row in `symbols/library_data.csv`, named as our side of the diff
  shows it, in place of a `STRING` marker.
- Scheduling inside one statement: one `fld` placed differently among x87 instructions, argument
  evaluation order inside one call, `fmulp` against `fmul` and `fstp`. `/G6` reproduces some of
  these and is a different scheduler; never ship it. Check first that the moved instructions are
  in the same block and the same statement.
- Pure register permutations. Where `esi` and `edi` trade places over a whole otherwise identical
  function, no source spelling, flag set or neighbouring function has been found to move them. The
  client sometimes gives `esi` to the range with fewer references, which VC6 SP3 never does on a
  probe. About a fifth of rows that read as permutations carry real differences once registers are
  normalised; `tools/residue_kinds.py` finds them.
- A constant the client commons and this build does not. `const`, plain and `register` locals all
  fold back. Check first that the client is not copying a struct. The reverse direction is
  reachable; see [Commoned constants](#commoned-constants).
- A commoned address. Where the client stores one function pointer into two slots through one
  register and this build repeats the immediate, or the reverse, no spelling reaches it.
- The twelve-byte x87 spill frame. Where the client reserves `sub esp, 0xc` for one spill and this
  build recycles a parameter slot, every spelling tried gives the same allocation.
- Some float sums. A three-term sum the client emits as y, z, x is not reachable, and neither is a
  product leaf whose base is still a pending `add`.
- Integer operand order. VC6 sorts the operands of a commutative integer sum by ascending offset,
  and no getter or named pointer overrides it. Base and index order in a SIB operand is not
  reachable where both addends are anonymous subexpressions. With both addends named, the second
  declared goes in the base.
- VC6 does not common `/` and `%` on the same operands.
- A vector result copied twice with x left on the x87 stack. Where the client keeps x on the
  stack, stores y and z to one vector, copies them into a second and stores x straight into the
  second, this build stores all three once: VC6 merges an inline `Vec3` return's local with its
  return slot, and no POD spelling (`Minus`, a free helper, `Copy()`, `Set`, a named copy,
  `operator=`) keeps them apart. Only a user copy constructor gives the client's two stores, and
  `Vec3` cannot have had one: VC6 then calls it out of line for every by-value argument, where the
  client copies each word inline, and it would bar `Vec3` from the network unions.
  `Object::ComputeSteeringDirection` shows the shape.
- A split that rounds an intermediate the client keeps on the x87 stack: see
  [Where a value is rounded](#where-a-value-is-rounded).
- Frame slot ties ([Slot order at /O2](#slot-order-at-o2)), and which variable takes a recycled
  parameter slot.
- Cross-jump survivors. Where the client keeps two copies of a block that differ only because its
  peephole pass ran before its cross-jumper, and this build merges them, nothing separates them.
  `-Og-` is the only switch that stops VC6 cross-jumping.
- Block placement VC6 derives from a proof. VC6 deletes a source test it has just proved, except
  through the loop-foot `break` in [Loops](#loops).
- A callee the client both inlines and calls in one unit ([The inline budget](#the-inline-budget)).
- Rows with several walls. A near miss can carry a recycled parameter slot, a SIB operand and a loop
  preamble at once. Its size says what closing it is worth, not whether it can close.

### Screens that look like walls and are not

- A zero-argument function on the scheduler wall: try the other calling convention.
- A register swap in a destructor: count the members with destructors.
- A frame permutation: check the reference counts and the names, as
  [Slot order at /O2](#slot-order-at-o2) says.
- Residue that is mostly jump displacements after one load: the load took `eax` (five bytes) on
  one side and another register (six bytes) on the other. A repeated read of one global rotates
  `eax`, `ecx`, `edx`, so count the reads on both sides before you name a local.
- An x87 leaf swap: try a by-value getter or a split statement.
- A permutation across two statements: swap the statements.
- The same instruction in two different blocks: it is a source fact.
- A mask width: change the consumer.
- Two recorded failures on one row: re-measure each on the current base before you believe
  either.
