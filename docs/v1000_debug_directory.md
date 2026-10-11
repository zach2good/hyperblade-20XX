# What v1.0.0.0 left in its debug directory

`HYPERX.EXE` v1.0.0.0 was linked with a debug directory. v1.0.0.1, which we match, has none.
Different compilers built the two, so nothing here transfers by address, but some of it transfers
by meaning.

## Contents

| Entry | Size | Content |
|---|---|---|
| MISC | 272 B | Module name `HyperX.exe` |
| CODEVIEW NB10 | 50 B | `C:\hyper\Build\Release\HyperX.pdb` |
| FPO | 67,872 B | 4242 `FPO_DATA` records |

The CODEVIEW path names the original build tree and shows that v1.0.0.0 was a Release build. It
still shipped with live asserts: the `Assertion failed: %s, file %s, line %d` string and the
just-in-time debugger prompt are in v1.0.0.0 and not in v1.0.0.1.

The FPO (frame pointer omission) table is the useful part. Each 16-byte record gives a function's start, its byte size,
its prologue length, the dwords of parameters and of locals in its frame, the number of saved
registers, and whether it uses SEH. All 4242 records have frame type `fpo`, so the table covers
only the functions without a frame pointer: 526,565 of the 1,017,854 bytes of `.text`. The
EBP-framed functions are absent.

## Reading the table

The three CSVs below are generated and not committed (`.gitignore` lists them). Make them in
order. The first:

    python tools/fpo_table.py "<v1.0.0.0 exe>" -o symbols/v1000_fpo.csv

This writes one row per record. The rows are sorted and do not overlap.

## Pairing v1.0.0.0 functions with v1.0.0.1 functions

### String literals

    python tools/fpo_anchors.py "<v1.0.0.0 exe>" "<v1.0.0.1 exe>" -o symbols/v1000_anchors.csv

String literals survive a recompile. The tool gives each literal reference in each image to the
function that contains it, then pairs the functions that own the same literals. It finds 125
anchors from 1926 shared literals.

The binary limits that figure. Its literal references are concentrated in a few very large
parsers: eight FPO functions hold over a thousand of the 2845 reference sites, and only 181 FPO
functions refer to a shared literal at all.

The matcher does not use size, so size is an independent check. Of the 125 pairs, 116 have sizes
within a quarter of each other. Random pairing puts about one pair in twelve inside that band.

### The call graph

    python tools/fpo_propagate.py "<v1.0.0.0 exe>" "<v1.0.0.1 exe>" -o symbols/v1000_propagated.csv

The same source built both images, so a function calls the same functions in both. Each paired
function votes for pairings between its unpaired callers on one side and its unpaired callers on
the other, and again for its callees. A witness with one candidate on each side casts a whole
vote. The tool takes a candidate when it strictly beats the runner-up, and rejects it when the
two sides call different imported DLL symbols.

Three properties of v1.0.0.0 shape the graph:

- It was linked incrementally. Its `.text` starts with 3042 five-byte `jmp rel32` thunks, and
  9902 of its 14542 direct calls go through one. The tool resolves each thunk to its target
  before it builds the graph. v1.0.0.1 has no thunks.
- The FPO table covers small functions (the median record is 31 bytes), and four call targets in
  five have no record. The tool adds each uncovered call target as a node of its own.
- Its `.text` is 1,017,854 bytes and v1.0.0.1's is 717,099, so many v1.0.0.0 functions have no
  counterpart.

Propagation takes the 125 literal anchors to 993, and size agreement stays about the same (about
nine pairs in ten within a quarter). The strict margin keeps it there: if the tool accepts ties,
the yield doubles and fewer than six pairs in ten agree on size.

## Argument counts

`param_dwords` is the number of dwords the callee finds on the stack. Register arguments are not
in it, so `__thiscall` loses `this` and `__fastcall` loses its first two. With that allowed for,
the record gives an argument count.

reccmp can't see this class of error. An unused trailing `__cdecl` parameter is invisible in the
callee, so a function can match with a signature that is short an argument. The FPO record still
counts it.

    python tools/fpo_arity.py [-p symbols/v1000_propagated.csv] [--source]
    python tools/check_call_arity.py              the written source
    python tools/check_call_arity.py --inventory  symbols/functions.csv

`fpo_arity.py` compares FPO records with the inventory or with the source. Treat a disagreement
as a place to look: the record describes v1.0.0.0, and a signature can change between builds.

`check_call_arity.py` needs no anchor. A `__cdecl` caller cleans up its own arguments, so
`add esp, N` after the call gives N/4 from v1.0.0.1 itself. It discards two kinds of site:

- VC6 merges the cleanup of adjacent calls, so a site counts only when the pushes in front of it
  account for all of N.
- A cleanup folded into the epilogue leaves nothing to read.

Every clean site of one callee must agree. Written source passes.

### Cases no byte count can settle

- **Folded stubs.** `Stub::Assert`, `KeyConfigDialog::SaveKeysAs` and `KeyConfigDialog::ClearKeys`
  are one, three and six bytes of `ret`. The linker folded several unrelated functions onto each, so one address
  is a vtable slot, a four-argument `__stdcall` and a two-argument `__cdecl`. No one signature
  fits, and the tree calls them through casts. Both tools skip bodies under eight bytes.
- **Structs by value.** `Object::SetVelocity` takes one `Vec3` by value, which is three stack
  dwords.
- **Hidden return pointers.** A function that returns a struct by value has one more stack dword
  than it has parameters.
- **Code Ghidra does not see.** Ghidra has no function over `0x00475510` to `0x00475c6f`, where
  the two netsetup dialog procedures live. A sweep over Ghidra's named functions misses their
  call sites. Handlers reached only through a function-pointer table have no direct call site;
  read the dispatcher found from the table's base address.

### The plan handler tables

The dispatchers into the plan handler tables at `0x004cf084` and `0x004cf2b8` push four dwords
and clean `0x10`, and the anchored handlers record four stack dwords. The tree declares the
handlers with four parameters.

The FPO records for a few functions disagree with v1.0.0.1 call sites. In each case the call
site decides. For example, `TeamBlock::TeamBlock()` at `0x00484730` records three stack dwords,
but its one caller at `0x00482ef8` pushes nothing.

`fpo_arity.py --source` over the literal anchors reports three disagreements that no call site
settles yet: `Network::AddPlayer` at `0x00440cc0`, `GameSession::ActivatePlayerSlot` at
`0x00440e90` and `GameSession::HandlePlayerDisconnect` at `0x00441000`. Each declares one more
argument than v1.0.0.0 records.
