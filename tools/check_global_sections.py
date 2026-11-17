"""Fails when a paired global sits in `.rdata` on one side and in a writable section on the other.

reccmp pairs globals by name and datacmp compares their values, so a pointer that should be
`const char* const` passes both while it lands in `.data`. The section says whether the original
declared it `const`.

    python tools/check_global_sections.py            # fail on a new mismatch or a stale row
    python tools/check_global_sections.py --save     # rewrite the register from the current build

`symbols/section_gaps.csv` names the mismatches kept on purpose. A registered row that now
agrees is a failure too, so the register cannot go stale.
"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REGISTER = ROOT / "symbols" / "section_gaps.csv"

HEADER = "address,name\n"


def engine():
    """The compare engine and both images, or a message naming what is missing."""
    try:
        from reccmp.compare import Compare
        from reccmp.project.detect import RecCmpProject
    except ImportError:
        raise SystemExit("reccmp is not installed; pip install reccmp")

    sys.path.insert(0, str(ROOT / "tools"))
    import foldcmp

    foldcmp.install()
    project = RecCmpProject.from_directory(ROOT)
    if not project:
        raise SystemExit("no reccmp project at %s" % ROOT)

    target = project.get("HYPERX1001")

    for path in (target.original_path, target.recompiled_path, target.recompiled_pdb):
        if not Path(path).is_file():
            raise SystemExit("%s is missing; build first" % path)

    return Compare.from_target(target), target.original_path, target.recompiled_path


def read_only(path):
    """The address ranges of an image's sections that the loader maps without write access."""
    import pefile

    image = pefile.PE(str(path), fast_load=True)
    base = image.OPTIONAL_HEADER.ImageBase
    ranges = []

    for section in image.sections:
        if section.Characteristics & pefile.SECTION_CHARACTERISTICS["IMAGE_SCN_MEM_WRITE"]:
            continue
        start = base + section.VirtualAddress
        ranges.append((start, start + section.Misc_VirtualSize))

    return ranges


def mismatches(compare, original, ours):
    """Every paired global whose section is read-only on exactly one side."""
    theirs_ro = read_only(original)
    ours_ro = read_only(ours)
    found = []

    for match in compare.get_variables():
        if not match.orig_addr or not match.recomp_addr:
            continue

        in_theirs = any(start <= match.orig_addr < end for start, end in theirs_ro)
        in_ours = any(start <= match.recomp_addr < end for start, end in ours_ro)
        if in_theirs != in_ours:
            found.append((match.orig_addr, match.name))

    return sorted(found)


def register():
    """The committed set of kept mismatches, keyed by original address."""
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
    # The register's comment block says why its rows are kept, so a save keeps it.
    comments = ["# Globals whose section differs from the original's.\n",
                "#\n",
                "# tools/check_global_sections.py fails when a paired global not named here is\n",
                "# read-only on one side only, and when a row here starts to agree.\n"]
    if REGISTER.exists():
        with open(REGISTER) as handle:
            comments = [line for line in handle if line.startswith("#")]

    with open(REGISTER, "w") as handle:
        handle.writelines(comments)
        handle.write(HEADER)
        for address, name in found:
            handle.write("%08x,%s\n" % (address, name))

    print("%d mismatch(es) written to %s" % (len(found), REGISTER))


def main():
    found = mismatches(*engine())

    if "--save" in sys.argv[1:]:
        save(found)
        return 0

    known = register()
    current = dict(found)

    new = [(address, name) for address, name in found if address not in known]
    closed = [(address, name) for address, name in known.items() if address not in current]

    for address, name in new:
        print("NEW         %08x %s" % (address, name))

    for address, name in sorted(closed):
        print("CLOSED      %08x %s agrees now; remove its row from %s" % (address, name, REGISTER))

    print("%d section mismatch(es), %d new, %d closed" % (len(found), len(new), len(closed)))
    return 1 if new or closed else 0


if __name__ == "__main__":
    raise SystemExit(main())
