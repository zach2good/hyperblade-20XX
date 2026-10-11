# Open work

[MATCHING.md](MATCHING.md) says how to match a function, [RE_CRUFT.md](RE_CRUFT.md) lists the
cruft shapes and what is left of each, and `python tools/progress.py` gives the current figures.

## Functions

- Unmatched functions. `python tools/fault_classes.py` sorts the rows that aren't exact by
  cause. Rows in the folding, data and x87 classes are mostly out of reach from source. Start
  with `python tools/fault_classes.py --open`, which ranks the rest by real differences.
- Unwritten functions. `python tools/ready_unwritten.py` ranks the functions with no marker by
  how much of what they call already exists. It needs the GhidraMCP bridge.
- Known program differences.
  [re_corrections.md](re_corrections.md#known-differences-the-source-does-not-reproduce) lists
  functions where the source keeps a spelling that differs from the original program, because
  the correct one matches fewer bytes. Each needs a spelling that gives both.
- Argument counts. Three functions declare one more argument than v1.0.0.0's FPO record:
  `Network::AddPlayer` (0x00440cc0), `GameSession::ActivatePlayerSlot` (0x00440e90) and
  `GameSession::HandlePlayerDisconnect` (0x00441000). No call site settles them yet.
  `python tools/fpo_arity.py --source` reports them; it needs the v1.0.0.0 tables, which
  `.gitignore` says how to build. See [v1000_debug_directory.md](v1000_debug_directory.md).
- `MeshBone::DrawFaces`. `this` and the vertex base take EBX and EDI the wrong way round.
  The client's static guard goes through `bl`, so EBX is free until the vertex base is defined.
  In the face loop the client shares EDI between `this` and the face and reloads `this` at the
  latch, where this build keeps `this` in the frame.

## Global data layout

reccmp pairs a direct `[global]` operand by name, so a marked global at the wrong address still
matches. A scaled index into a table reccmp cannot name pairs only when both tables sit at the
same address, so those rows need the data at the original's address.
[MATCHING.md](MATCHING.md#data-globals-and-link-order) gives the layout rules.

The remaining rows whose only fault is a data address are blocked by data the tree can't place.
They read the CRT's storage, such as `__iob` and `__pi_by_2_to_61`, or pass the address of a
function-scope static, which VC6 gives no symbol to mark. `EngineObject::UpdateRenderSlotTransform`,
its twin in `PlayerObject` and `MeshBone::DrawFaces` are the static cases.

## Other modules on the disc

`GET3D.DLL` is the only other module worth reversing, and it needs a VC 4.0 toolchain we don't
have. See [OTHER_MODULES.md](OTHER_MODULES.md).

## Out of scope

This repository reproduces `HYPERX.EXE` byte for byte, so it can't change calling conventions,
memory layouts or API use, and modernising the engine is out of scope.
