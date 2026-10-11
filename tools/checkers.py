"""Run every structural checker and print one line each.

The `verify` build target also runs checkers, but a failure there is buried in build output.
This runs the tree's structural checkers directly and prints a short table. It does not build
or compare bytes.

    python tools/checkers.py            # one line each, failures last
    python tools/checkers.py --full     # the full output of every checker that failed

Exit code 1 when any checker fails.
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Byte and resource comparisons are left out: they need the reference image, and `verify` runs
# them. So is check_vtable_arity_declared.py, which reads the reference, and
# check_conventions.py, which needs the Ghidra bridge.
CHECKERS = ("check_field_offsets.py", "check_field_tiling.py", "check_member_owner.py",
            "check_self_casts.py", "check_slot_arity.py", "check_tables_named.py",
            "check_virtual_bodies.py", "check_vptr_offsets.py", "check_dtor_calls.py",
            "check_body_reach.py", "shared_reserves.py", "base_reads_derived.py",
            "check_vtbl_stores.py",
            "check_self_assign.py",
            "check_dead_stores.py",
            "check_balance.py",
            "check_record_arith.py",
            "check_dead_this_alias.py",
            "check_unassigned_locals.py",
            "check_marker_comments.py",
            "check_score_prose.py",
            "check_volatile.py",
            "check_asm_bodies.py",
            "view_classes.py --twice",
            "index.py --check",
            "check_doc_paths.py",
            "check_dead_defines.py",
            "check_chained_casts.py",
            "check_duplicate_constants.py",
            "check_scaled_offsets.py",
            "check_macro_enum_clash.py",
            "check_discarded_types.py",
            "check_mixed_units.py",
            "redundant_return_casts.py",
            "self_type_casts.py",
            "base_this_casts.py",
            "check_class_struct_keyword.py",
            "implicit_narrowing.py",
            "check_null_test_casts.py",
            "dead_defines.py",
            "check_cast_width.py",
            "check_stray_sources.py",
            "check_globals.py",
            "check_explicit_ctors.py",
            "check_declaration_style.py",
            "check_include_hygiene.py",
            "check_ghost_forwards.py",
            "check_self_naming_notes.py",
            "check_ledger_figures.py",
            "check_retired.py",
            "check_link_order.py",
            "const_locals.py",
            "const_accessors.py",
            "unused_params.py",
            "check_named_addresses.py",
            "check_id_notes.py",
            "check_source_paths.py",
            "unused_includes.py",
            "unused_includes.py --headers",
            "check_duplicated_prose.py",
            "check_doubled_file_names.py",
            "check_undefined_members.py",
            "check_restated_externs.py",
            "check_extern_bounds.py",
            "check_redundant_forwards.py",
            "check_note_scars.py",
            "kind_names.py",
            "rival_names.py",
            "blob_rivals.py",
            "plan_names.py",
            "enum_names.py",
            "marker_next_line.py",
            "check_lint.py",
            "check_prefixed_functions.py",
            "namespace_spread.py",
            "header_homes.py",
            "placeholder_globals.py --max 0",
            "placeholder_functions.py --max 0",
            "x87_precision.py --selftest",
            "plan_dump.py --selftest")


def refresh():
    """Regenerate build/report.json when the image is newer than it.

    One checker reads the report, and tools/report.py refuses a stale one. This takes about a
    minute.
    """
    sys.path.insert(0, os.path.join(ROOT, "tools"))
    import report

    if not report.stale():
        return

    print("report.json is behind the binary; regenerating it first")
    subprocess.run([sys.executable, "-m", "reccmp.tools.asmcmp", "--target", "HYPERX",
                    "--json", os.path.join("build", "report.json"), "--silent"],
                   cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def main():
    full = "--full" in sys.argv[1:]
    failed = []

    refresh()

    for entry in CHECKERS:
        parts = entry.split()
        result = subprocess.run([sys.executable, os.path.join(ROOT, "tools", parts[0])]
                                + parts[1:], cwd=ROOT, capture_output=True, text=True)
        lines = [line for line in result.stdout.splitlines() if line.strip()]
        # The last line every one of these prints is its own count.
        summary = lines[-1] if lines else "(no output)"

        print("%-4s %-28s %s" % ("ok" if result.returncode == 0 else "FAIL", entry, summary[:88]))

        if result.returncode != 0:
            failed.append((entry, result.stdout))

    if failed:
        print("")
        print("%d checker(s) failed: %s" % (len(failed), " ".join(name for name, unused in failed)))

        if full:
            for name, out in failed:
                print("")
                print("=== %s" % name)
                print(out)

        return 1

    print("")
    print("%d checker(s), all clean" % len(CHECKERS))

    return 0


if __name__ == "__main__":
    sys.exit(main())
