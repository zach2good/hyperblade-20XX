"""Fails when a global's value stops agreeing with the original's.

reccmp walks every GLOBAL marker, resolves its type from the PDB and compares the bytes of both
images field by field. The byte score reads only code, and tools/check_initialised_globals.py
only looks for a value this tree leaves at zero; this catches any difference, such as a global
uninitialised in the original and 1 here.

The register of known differences is symbols/data_gaps.csv. Its reason column says why each
one is benign; --save keeps the reasons of the rows that remain.

    python tools/check_datavalues.py           # fail on a new difference or a stale row
    python tools/check_datavalues.py --list    # print the field diff for every difference
    python tools/check_datavalues.py --save    # rewrite the register from the current build

Only DIFF and ERROR are recorded. `python -m reccmp.tools.datacmp` reports a variable it can
read but not type as WARN; that is a gap in the type, not a difference in the value.
"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REGISTER = ROOT / "symbols" / "data_gaps.csv"


def target():
    """The reccmp target, or a message naming what is missing."""
    try:
        from reccmp.project.detect import RecCmpProject
    except ImportError:
        raise SystemExit("reccmp is not installed; pip install reccmp")

    project = RecCmpProject.from_directory(ROOT)
    if not project:
        raise SystemExit("no reccmp project at %s" % ROOT)

    found = project.get("HYPERX")

    for path in (found.original_path, found.recompiled_path, found.recompiled_pdb):
        if not Path(path).is_file():
            raise SystemExit("%s is missing; build first" % path)

    return found


def differences(verbose):
    """Every variable whose value differs, or that reccmp could not read, by address."""
    from reccmp.compare.variables import CompareResult
    from reccmp.tools.datacmp import do_the_comparison

    found = []

    for item in do_the_comparison(target()):
        if item.result not in (CompareResult.DIFF, CompareResult.ERROR):
            continue

        found.append((item.orig_addr, item.name, item.result.name))

        if verbose:
            print("%s (0x%x) ... %s" % (item.name[:80], item.orig_addr, item.result.name))
            if item.error is not None:
                print("  %s" % item.error)
            for compared in item.compared:
                if not compared.match:
                    print("  + 0x%02x %-30s %s : %s"
                          % (compared.offset, compared.name or "", compared.values[0], compared.values[1]))

    return sorted(found)


def register():
    if not REGISTER.exists():
        return {}

    known = {}

    with open(REGISTER) as handle:
        for line in handle:
            if line.startswith("#") or line.startswith("address,") or not line.strip():
                continue
            parts = line.strip().split(",")
            known[int(parts[0], 16)] = parts[1] if len(parts) > 1 else ""

    return known


def save(found):
    reasons = {}

    if REGISTER.exists():
        with open(REGISTER) as handle:
            for line in handle:
                parts = line.rstrip("\n").split(",", 3)
                if len(parts) == 4 and not line.startswith("#"):
                    reasons[parts[0]] = parts[3]

    with open(REGISTER, "w") as handle:
        handle.write("# Globals whose value does not agree with the original's.\n")
        handle.write("#\n")
        handle.write("# tools/check_datavalues.py fails when a global not named here starts to\n")
        handle.write("# differ, and when one named here stops. Remove the row in the same change\n")
        handle.write("# that closes the gap.\n")
        handle.write("# The reason column says why the difference is benign.\n")
        handle.write("address,name,result,reason\n")
        for address, name, result in found:
            handle.write("%08x,%s,%s,%s\n" % (address, name, result, reasons.get("%08x" % address, "")))

    print("%d difference(s) written to %s" % (len(found), REGISTER))


def main():
    verbose = "--list" in sys.argv[1:]
    found = differences(verbose)

    if "--save" in sys.argv[1:]:
        save(found)
        return 0

    known = register()
    current = {address: name for address, name, _ in found}

    new = [(address, name) for address, name, _ in found if address not in known]
    closed = [(address, name) for address, name in known.items() if address not in current]

    for address, name in sorted(new):
        print("NEW DIFF    %08x %s" % (address, name[:60]))

    for address, name in sorted(closed):
        print("AGREES      %08x %s agrees now; remove its row from %s" % (address, name[:60], REGISTER))

    print("%d value difference(s), %d new, %d closed" % (len(found), len(new), len(closed)))
    return 1 if new or closed else 0


if __name__ == "__main__":
    raise SystemExit(main())
