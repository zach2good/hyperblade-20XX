"""Fails when a vtable that used to pair against the original stops pairing.

The byte score reads code. A vtable is data, so a slot in the wrong order, a slot holding
`_purecall` where the original holds a body, or a missing virtual that shifts every slot below
it, costs nothing in the headline number.

reccmp compares vtables, and `python -m reccmp.tools.vtable` prints the diff, but it cannot
tell a table that never paired from one that stopped. This checks against
`symbols/vtable_gaps.csv`, which names the tables known not to pair.

    python tools/check_vtables.py            # fail on a new gap or a stale row
    python tools/check_vtables.py --list     # print the diff for every gap
    python tools/check_vtables.py --save     # rewrite the register from the current build

A registered table that now pairs is a failure too, so the register cannot go stale.
"""
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REGISTER = ROOT / "symbols" / "vtable_gaps.csv"
IMAGE = ROOT / "build" / "out" / "HYPERX_sym.exe"

HEADER = "address,name\n"


def engine():
    """The compare engine, or a message naming what is missing.

    Needs the linked symbol build and the original, neither of which is in the repository.
    """
    try:
        from reccmp.compare import Compare
        from reccmp.project.detect import RecCmpProject
    except ImportError:
        raise SystemExit("reccmp is not installed; pip install reccmp")

    project = RecCmpProject.from_directory(ROOT)
    if not project:
        raise SystemExit("no reccmp project at %s" % ROOT)

    target = project.get("HYPERX")

    for path in (target.original_path, target.recompiled_path, target.recompiled_pdb):
        if not Path(path).is_file():
            raise SystemExit("%s is missing; build first" % path)

    return Compare.from_target(target)


def stale():
    """Source files newer than the image the comparison reads.

    A source edited after the last link shifts its line numbers against the PDB, and reccmp
    can drop the function a slot points at, which reads as a slot that stopped pairing.
    """
    if not IMAGE.exists():
        return []

    linked = IMAGE.stat().st_mtime
    late = []

    for root, _, files in os.walk(ROOT / "src"):
        for name in files:
            if name.endswith((".c", ".cpp", ".h")):
                path = Path(root) / name
                if path.stat().st_mtime > linked:
                    late.append(path)

    return late


def gaps(compare, verbose):
    """Every vtable that does not pair, in original-address order."""
    from reccmp.compare.diff import raw_diff_to_udiff
    from reccmp.utils import print_combined_diff

    found = []

    for match in compare.compare_vtables():
        if match.ratio >= 1:
            continue

        found.append((match.orig_addr, match.name))

        if verbose:
            print("%s : orig 0x%x, recomp 0x%x" % (match.name, match.orig_addr, match.recomp_addr))
            print_combined_diff(raw_diff_to_udiff(match.result.diff), True)
            print()

    return sorted(found)


def register():
    """The committed set of known gaps, keyed by original address."""
    if not REGISTER.exists():
        return {}

    known = {}

    with open(REGISTER) as handle:
        for line in handle:
            if line.startswith("#") or line.startswith("address,") or not line.strip():
                continue
            parts = line.strip().split(",", 1)
            known[int(parts[0], 16)] = parts[1] if len(parts) > 1 else ""

    return known


def save(found):
    with open(REGISTER, "w") as handle:
        handle.write("# Vtables that do not pair against the original.\n")
        handle.write("#\n")
        handle.write("# tools/check_vtables.py fails when a table not named here stops pairing, and\n")
        handle.write("# when a table named here starts. Remove the row in the same change that\n")
        handle.write("# closes the gap.\n")
        handle.write(HEADER)
        for address, name in found:
            handle.write("%08x,%s\n" % (address, name))

    print("%d gap(s) written to %s" % (len(found), REGISTER))


def main():
    verbose = "--list" in sys.argv[1:]
    found = gaps(engine(), verbose)

    if "--save" in sys.argv[1:]:
        save(found)
        return 0

    late = stale()
    if late:
        print("STALE BUILD: %d source file(s) are newer than %s." % (len(late), IMAGE))
        print("REBUILD before believing anything below.")

    known = register()
    current = dict(found)

    new = [(address, name) for address, name in found if address not in known]
    closed = [(address, name) for address, name in known.items() if address not in current]

    for address, name in sorted(new):
        print("NEW GAP     %08x %s" % (address, name))

    for address, name in sorted(closed):
        print("CLOSED      %08x %s pairs now; remove its row from %s" % (address, name, REGISTER))

    print("%d vtable gap(s), %d new, %d closed" % (len(found), len(new), len(closed)))
    return 1 if new or closed else 0


if __name__ == "__main__":
    raise SystemExit(main())
