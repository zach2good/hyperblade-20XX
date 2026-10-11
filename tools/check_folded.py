"""Report identical-code folding in the measured build, and annotations that disagree about it.

When two functions have identical bytes, the linker can keep one copy at one address. reccmp's
marker `// FUNCTION: HYPERX 0xADDRESS FOLDED` says that several annotations legitimately
resolve to the same recompiled address.

`hyperx_sym` is linked `/OPT:NOREF`, so no folding is expected, and an unannotated fold means
the link flags changed. The tool exists for linking with folding on, as the original was
(`/OPT:REF`, which in VC6 implies folding).

    python tools/check_folded.py            # fail on an inconsistent or unmatched annotation
    python tools/check_folded.py --list     # print every folded address and its symbols

It fails on:

- annotations at one address that disagree about FOLDED, so reccmp's view depends on which it
  reads first;
- a FOLDED annotation that matches no recompiled function, so it scores nothing;
- a FOLDED annotation that matches more than one, so the same bytes count twice;
- folding in the build while no annotation in the tree says FOLDED.

Ported from the LEGO Racers decompilation.
"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def engine():
    """The compare engine, or a message naming what is missing."""
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


def folded_addresses(compare):
    """Recompiled addresses carrying more than one symbol, which is what folding looks like."""
    from reccmp.formats import PEImage

    assert isinstance(compare.recomp_bin, PEImage)
    seen = {}

    for symbol in compare.cvdump_analysis.parser.symbols:
        address = compare.recomp_bin.get_abs_addr(symbol.section, symbol.offset)
        seen.setdefault(address, []).append(symbol)

    return {key: value for key, value in seen.items() if len(value) > 1}


def annotations(compare):
    """Every FUNCTION annotation this tree carries, grouped by original address."""
    from reccmp.parser import DecompCodebase

    codebase = DecompCodebase(compare.code_files, compare.target_id)
    codebase.prune_invalid_addrs(compare.orig_bin.is_valid_vaddr)
    codebase.prune_reused_addrs()

    grouped = {}

    for function in codebase.iter_line_functions():
        grouped.setdefault(function.offset, []).append(function)

    for function in codebase.iter_name_functions():
        grouped.setdefault(function.offset, []).append(function)

    return grouped


def main():
    compare = engine()
    folded = folded_addresses(compare)

    if "--list" in sys.argv[1:]:
        for address, symbols in sorted(folded.items()):
            # pylint: disable=protected-access
            from reccmp.types import ImageId
            entry = compare._db.get(ImageId.RECOMP, address)
            original = entry.orig_addr if entry is not None else None
            print("orig %8s  recomp %#08x  (%d symbols)"
                  % ("%#08x" % original if original else "-", address, len(symbols)))
            for symbol in symbols:
                print("    %s" % symbol.name)
            print()

    faults = 0

    for address, functions in sorted(annotations(compare).items()):
        marked = [f.is_folded for f in functions]
        if any(marked) and not all(marked):
            print("%08x: %d annotation(s) say FOLDED and %d do not"
                  % (address, sum(marked), len(marked) - sum(marked)))
            faults += 1

    for address, functions in sorted(annotations(compare).items()):
        # A nameref annotation, such as a scalar destructor, resolves by name and is skipped.
        claimed = [f for f in functions if f.is_folded and not f.is_nameref()]

        if not claimed:
            continue

        # pylint: disable=protected-access
        matched = [
            compare._lines_db.find_function(f.filename, f.line_number, f.end_line, folded=True)
            for f in claimed
            if f.filename is not None
        ]
        found = len([m for m in matched if m])

        if found == 0:
            print("%08x: FOLDED annotation matches no recompiled function" % address)
            faults += 1
        elif found > 1:
            print("%08x: FOLDED annotation matches %d recompiled functions" % (address, found))
            faults += 1

    # hyperx_sym links /OPT:NOREF, so an unannotated fold means the link flags changed. A folded
    # function loses its symbol and stops scoring.
    declared = any(f.is_folded for group in annotations(compare).values() for f in group)

    if folded and not declared:
        print("%d folded address(es) and not one FOLDED annotation in the tree." % len(folded))
        print("hyperx_sym is meant to link /OPT:NOREF. Check its link options before believing")
        print("any score from this build: a folded function loses its symbol and stops counting.")
        faults += 1

    print("%d folded address(es) in the build, %d annotation fault(s)" % (len(folded), faults))
    return 1 if faults else 0


if __name__ == "__main__":
    raise SystemExit(main())
