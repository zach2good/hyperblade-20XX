# Annotations and registers

reccmp pairs our functions and data with the original's through comment markers in `src/`. This
document covers the marker rules, the faults a misplaced marker causes, and the CSV registers
under `symbols/` where a measured exception goes. [MATCHING.md](MATCHING.md) holds the matching
technique.

## Markers

Each marker is one line, `// KIND: HYPERX 0xADDRESS`, alone on its line and directly above what it names.

| Marker | Names | Rule |
|---|---|---|
| `FUNCTION` | a function this tree writes | above the definition or header inline |
| `GLOBAL` | a global variable | above the `extern` or the definition |
| `STRING` | a string literal in `.rdata` | above the line that holds the literal |
| `VTABLE` | a class's virtual table | above the `class` or `struct` line |
| `LIBRARY` | a function linked from `LIBCMT.LIB` or IJG JPEG | by name, in a header |
| `SYNTHETIC` | a compiler-generated function | by name |

A by-name marker takes its name from the comment line under it: the mangled or C name for
`LIBRARY`, and a name such as ``AnimBinding::`scalar deleting destructor'`` for `SYNTHETIC`.

`src/common/library.h` holds the CRT's `LIBRARY` markers, and `src/jpeg/markers.h` the JPEG
library's. By-name markers go in headers: reccmp's `decomplint` reports one in a `.cpp` file as
`byname_function_in_cpp`.

`FUNCTION: HYPERX 0xADDR FOLDED` tells reccmp that several markers may claim one recompiled
address, for a build that folds our own bodies. It can't restore a symbol the linker discarded,
and it does nothing for the original's folds.

### A marker binds to the next declaration

reccmp reads a marker's name from the line immediately under it. On the original side it puts
that name into the disassembly; on our side the name comes from the PDB. It compares the two as
text, which leads to these faults:

- A comment between a marker and its declaration becomes the symbol's name. Every instruction
  that names the symbol then fails to pair, and the report shows an operand printed as the
  comment's text. Put prose above the marker. By-name markers use this on purpose: the line
  under a `LIBRARY` or `SYNTHETIC` marker is the name.
- A declaration inserted between a class's marker and the class takes the marker. A new struct
  put above `class X` lands under X's `VTABLE` marker, the vtable row vanishes, and the
  constructor and destructor drift. Insert above the whole comment block that precedes the
  class, or after its closing `};`.
- A forward declaration written between a `GLOBAL` marker and its `extern` leaves the marker
  unresolved, and reccmp scores every instruction naming that address as a mismatch. It looks
  like a byte price for whatever else the change did.
- A `STRING` marker must sit directly above the line with the literal. A misplaced one fails to
  parse and takes the enclosing function's marker with it, so the whole row vanishes. A call
  whose literal is split across two lines under a `STRING` marker does the same.
- A marker folded into prose by a rewrap is gone. reccmp doesn't see it, and
  `check_markers_resolve.py` has nothing left to report. `python tools/marker_next_line.py`
  fails on both faults.

### Where a marker sits can change a score

- A marker is worth what the pairing gives. Moving one to its correct function can make one row
  exact and cost another more. Measure both before you move it.
- Naming an address can cost. A `GLOBAL` for a table that reccmp otherwise shows as a
  placeholder can stop anonymous `call` lines pairing.
- A `GLOBAL` marker doesn't make a scaled-index operand pair. Only the address does.
- reccmp ends each original function at the next annotated address. A marker on a stray `ret`
  or on alignment padding truncates its neighbour or reads padding as code. Where a fault sits
  at a function's first or last instruction, check the boundary.
- One address takes one marker. A `VTABLE` and a `GLOBAL` on one address drop one of them
  without a message. Give a vtable address to the class it is worth most to.
- A vtable store is an address immediate. reccmp translates it only through a `VTABLE` marked
  class or a `GLOBAL` marked placeholder.
- Write every address as `0x` and eight hex digits.
- An `extern` of an array with no bound gives the global a PDB size of nought. reccmp then
  cannot name an offset inside it and prints a numbered placeholder, which pairs only by
  chance. Write the bound in every `extern`, `extern int g_anKeyConfigKeys[10];`.

### Operand residue

A line can carry the client's bytes and still fail because reccmp names an operand differently
on each side. `python tools/operand_residue.py` pairs each row's differing lines, blanks every
symbol and image address, and lists the rows whose residue is only operand names, sorted by
cause. `--only` keeps the rows with no code line left. Most such rows are walls:

- `bare`: neither side has a symbol. LIBCMT's static data and the C++ function-local statics
  have none in our PDB, and VC6 writes the guard byte but not the object. A one-past-the-end
  loop bound lands on whatever the linker put next, which differs between the images.
- `rival`: both sides are named, differently. For a call or a function pointer this is the
  client's identical-COMDAT folding (`Stub::Assert`, `Object::Trace`, the `PlanArg` getters).
  The `$E` thunks of each number share one name and pair in address order, so the link order
  decides which object's thunk a marker takes ([Data, globals and link order](MATCHING.md#data-globals-and-link-order)).

The fixable causes are wrong annotations, and the checks below fail on each:

- **A marker on the wrong library datum.** LIBCMT's data is the same object code in both
  images, so its bytes must agree. `??_R0?AVtype_info@@@8` sat at `0x004ca340`, one past `_iob`,
  and `__lpdays` and `__days` were four bytes low. Moving them made `__ftime` and
  `___loctotime_t` exact.
- **A float row for a constant our PDB cannot name.** A row in `symbols/float_constants.csv`
  makes the client read `[1.0 (FLOAT)]`. LIBCMT's own constants have no symbol here, so our
  side reads a placeholder. Without the row both sides read placeholders and pair. Dropping the
  seven rows at `0x004b4810` made `__check_range_exit` and `__convertTOStoQNaN` exact.

A `jmp dword ptr [reg*4 + table]` that renders raw on the client and as a placeholder here is a
symptom. The client's function entity takes our size, so its table falls outside it until the
code before the table is the same length.

### Checks

`check_markers_resolve.py` also fails on two faults the operand scan found:

- `WRONG BYTES`: an original address paired with LIBCMT data whose bytes differ from ours,
  other than two image addresses in one dword.
- `FLOAT`: an operand that reads `(FLOAT)` on one side and a placeholder on the other, from
  `build/report.json`. It catches a missing float row and a row for a library constant.

```sh
python tools/check_markers_resolve.py      # needs a current build and build/report.json
python tools/marker_next_line.py
python tools/duplicate_markers.py
python -m reccmp.tools.decomplint --target HYPERX src
```

## Where exceptions are recorded

A measured refusal goes in a CSV under `symbols/`, not in prose. A row there must carry the
measurement. A row without evidence is worse than none: it hides live work from every list that
reads the register.

Each file under `symbols/` opens with a comment that says what it holds, except the three whose
readers take the first line as the header:

| File | Holds |
|---|---|
| `functions.csv` | every function in the original: address, Ghidra's name, size, convention and signature, from `tools/pull_functions.py` |
| `doc_paths.csv` | paths the docs name on purpose that no longer exist, for `tools/check_doc_paths.py` |
| `vtable_overruns.csv` | classes that declare more virtuals than their table holds, written by `tools/check_vtable_arity_declared.py --save` |

The other registers each name the tool that fails on a row they do not list.

### `symbols/retired.csv`

Columns: `address,name,wall,evidence`. One row per function whose remaining gap was measured and
given up.

- `address` is the function's address, `0x` and eight hex digits.
- `name` is the function's name in the tree.
- `wall` is the class of residue, such as `scheduler`, `allocator`, `layout`,
  `mid-struct-rebase`, `commoned-nought`, `call-table` or `comdat-fold`.
- `evidence` says what was tried and what each spelling measured. A failed spelling goes here.

A retired function is written and linked, and behaves as the original. Only the attempt on its
last instructions is retired. Read a row as "not reachable with the levers known when it was
written". When a new lever lands, re-test the rows on the walls it could reach.
`tools/near_misses.py` leaves retired rows out; pass `--retired` to list them. `tools/check_regressions.py` fails with a STALE
RETIREMENT line when a retired row is exact, so remove the row when you close it.

Count rows with `awk -F, '/^0x/' symbols/retired.csv`, not `grep -c`: the header is prose.

`symbols/blocked_rows.csv` (`address,name,reason,file`) holds rows no spelling can move: a reccmp
limit or a measured wall, such as link folding. A refusal measured by trying spellings goes in
`symbols/retired.csv`. `tools/near_misses.py` excludes both.

### `symbols/layout_exceptions.csv`

Columns: `path,reason`. The layout rule is one class to one `.cpp` file. A file listed here keeps
a body of a class whose own file is elsewhere, because moving it would make a different
translation unit from the original's. `tools/class_spread.py` and `tools/namespace_spread.py`
read the list.

### Hand-written assembly

Columns of `symbols/original_asm.csv`: `address,name,evidence`. A routine goes here only when the
original's own instructions show that a person wrote them. These idioms are the proof, because
VC6 never emits them:

- `PUSHAD`/`POPAD`, or `PUSHF`/`POPF`.
- EBP used as a data register inside a function that has a frame.
- ESP saved and restored by hand, as `push esp` and `pop esp`.
- Dead loads in an unrolled fill: a value read into a register that nothing reads again.
- Values passed between the arms of a routine by `push` and `pop`.
- A routine that reads its caller's frame through the caller's EBP, or returns in the flags.
- `REP STOSW` or `REP MOVSW`, `MOV reg, 0` at `/O2`, and a branch on the carry of a shift.

A frame pointer alone isn't proof. One idiom from the list is.

Write the routine as the original did: `__declspec(naked)` where the original has no compiler
prologue, and a C++ function with an `__asm` block where the compiler made the frame, the
parameters or the code round the block. Keep the original's instruction order. Name labels,
parameters and globals for what they are. Directly above the marker, write this block:

```cpp
// Hand-written assembly in the original: <the idioms that prove it, one line each>.
// What it does: <the routine in plain words>.
// Why assembly: <what C cannot express, such as the register or frame contract>.
// FUNCTION: HYPERX 0x0044af70
```

`tools/check_asm_bodies.py` fails on an `__asm` or `__declspec(naked)` outside a function in the
register, or one without the three lines. A row that becomes exact leaves `symbols/retired.csv`.

### Notes in the source

A file's header comment may record why a body is where it is or why a pragma is there, with the
measured price. Keep it to the fact. A spelling that failed goes in `symbols/retired.csv`, not in
the comment.
