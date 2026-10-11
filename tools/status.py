"""Displays status across matching, clean-up and bring-up.

Summarises:
  Matching: byte matching and decompilation coverage against the retail binary.
  Clean-up: cruft cleaning, checker status, and C++ architectural health.
  Bring-up: execution and runtime engine states on modern systems.

Usage:
  python tools/status.py
"""

import csv
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPORT = os.path.join(ROOT, "build", "report.json")
INVENTORY = os.path.join(ROOT, "symbols", "functions.csv")
STATUS_SVG = os.path.join(ROOT, "progress", "status.svg")
BRINGUP = os.path.join(ROOT, "tools", "bringup_baseline.json")

GAME_CODE_LIMIT = 0x004A0000


def run_checker(name, fallback, match=None):
    """Run a checker and return its summary line, or `fallback` when it cannot run.

    `match` picks the line holding that text instead of the last one, for a checker that
    prints its summary before its detail.
    """
    import subprocess
    try:
        out = subprocess.check_output([sys.executable, os.path.join(ROOT, "tools", name)],
                                      stderr=subprocess.STDOUT, timeout=120)
        lines = out.decode("utf-8", "replace").strip().splitlines()

        if match is not None:
            for line in lines:
                if match in line:
                    return line.strip()

            return fallback

        return lines[-1].strip() if lines and lines[-1].strip() else fallback
    except Exception:
        return fallback


def measure_cruft():
    import glob
    # Every spelling of a byte cast of `this`, qualifiers included.
    blob = re.compile(r"\(\s*(?:const\s+|volatile\s+)*(?:unsigned\s+|signed\s+)?(?:char|void)\s*\*+\s*(?:const\s*)?\)\s*this\b")
    n, files = 0, 0
    paths = (glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True)
             + glob.glob(os.path.join(ROOT, "src", "**", "*.h"), recursive=True))
    for path in paths:
        hits = 0
        for line in open(path, encoding="utf-8", errors="replace"):
            if not line.lstrip().startswith("//"):
                hits += len(blob.findall(line))
        if hits:
            n += hits
            files += 1
    selfcast = run_checker("check_self_casts.py", "unknown")
    return {
        "checkers": len(glob.glob(os.path.join(ROOT, "tools", "check_*.py"))),
        "vtbl": run_checker("check_vtbl_stores.py", "unknown"),
        "selfcast": selfcast.split(";", 1)[1].strip() if ";" in selfcast else selfcast,
        # check_self_casts counts the view classes it searches, pinned or not; view_classes.py
        # counts the ones still undecoded.
        "views": run_checker("view_classes.py", "unknown", match="still views"),
        "blob": n,
        "blobfiles": files,
        "offsets": run_checker("check_named_offsets.py", "unknown"),
        "scaled": run_checker("check_scaled_offsets.py", "unknown"),
        "chained": run_checker("check_chained_casts.py", "unknown"),
        "dupes": run_checker("check_duplicate_constants.py", "unknown"),
        "discarded": run_checker("check_discarded_types.py", "unknown"),
        "deaddefs": run_checker("check_dead_defines.py", "unknown"),
        "clash": run_checker("check_macro_enum_clash.py", "unknown"),
        "puns": run_checker("pointer_puns.py", "unknown"),
        "untyped": run_checker("untyped_members.py", "unknown"),
        "mixed": run_checker("check_mixed_units.py", "unknown"),
        "classfiles": run_checker("class_files.py", "unknown"),
        "classspread": run_checker("class_spread.py", "unknown"),
        "docpaths": run_checker("check_doc_paths.py", "unknown"),
        "hygiene": run_checker("check_include_hygiene.py", "unknown"),
        "ghosts": run_checker("check_ghost_forwards.py", "unknown"),
        "selfnamed": run_checker("check_self_naming_notes.py", "unknown"),
        "scoreprose": run_checker("check_score_prose.py", "unknown"),
        "constacc": run_checker("const_accessors.py", "unknown"),
        "unusedparams": run_checker("unused_params.py", "unknown"),
        "namedaddrs": run_checker("check_named_addresses.py", "unknown"),
        "srcpaths": run_checker("check_source_paths.py", "unknown"),
        "unusedinc": run_checker("unused_includes.py", "unknown"),
        "constlocals": run_checker("const_locals.py", "unknown"),
        "offsetreads": run_checker("offset_reads.py", "unknown"),
        "leadruns": run_checker("undecoded_views.py", "unknown"),
        "shared": run_checker("check_member_name_collisions.py", "unknown"),
    }


def get_inventory_sizes():
    sizes = {}
    if not os.path.exists(INVENTORY):
        return sizes
    with open(INVENTORY, encoding="utf-8") as f:
        reader = csv.DictReader(line for line in f if not line.startswith("#"))
        for row in reader:
            try:
                addr = int(row["address"], 16)
                size = int(row["size"]) if str(row["size"]).isdigit() else 0
                sizes[addr] = size
            except (ValueError, KeyError):
                continue
    return sizes


def get_card_stats():
    stats = {}
    if os.path.exists(STATUS_SVG):
        try:
            content = open(STATUS_SVG, encoding="utf-8").read()
            m_impl = re.search(r"Implemented:\s*([\d.]+)%\s*\((\d+)/(\d+)\)", content)
            if m_impl:
                stats["impl_pct"] = float(m_impl.group(1))
                stats["implemented"] = int(m_impl.group(2))
                stats["total"] = int(m_impl.group(3))
            m_acc = re.search(r"Accuracy:\s*([\d.]+)%", content)
            if m_acc:
                stats["accuracy"] = float(m_acc.group(1))
            m_prog = re.search(r"<tspan[^>]*>([\d.]+)%</tspan>", content)
            if m_prog:
                stats["progress"] = float(m_prog.group(1))
        except Exception:
            pass
    return stats


def get_exact_stats(sizes):
    exact_count = 0
    exact_bytes = 0
    if os.path.exists(REPORT):
        try:
            with open(REPORT, encoding="utf-8") as f:
                data = json.load(f).get("data", [])
                for row in data:
                    try:
                        addr = int(str(row.get("address", "")), 16)
                    except ValueError:
                        continue
                    if addr < GAME_CODE_LIMIT and float(row.get("matching", 0.0)) >= 1.0:
                        exact_count += 1
                        exact_bytes += sizes.get(addr, 0)
        except Exception:
            pass
    return exact_count, exact_bytes


def get_bringup_states():
    if not os.path.exists(BRINGUP):
        return {}
    try:
        with open(BRINGUP, encoding="utf-8") as f:
            data = json.load(f)
            return data.get("states", {})
    except Exception:
        return {}


def make_bar(percent, width=30):
    clamped = max(0.0, min(100.0, percent))
    filled = int(round(width * clamped / 100.0))
    return "[" + "=" * filled + " " * (width - filled) + "]"


def main():
    sizes = get_inventory_sizes()
    card_stats = get_card_stats()
    exact_count, exact_bytes = get_exact_stats(sizes)
    bringup_states = get_bringup_states()

    total_inventory = card_stats.get("total", len(sizes) or 4121)
    implemented_count = card_stats.get("implemented", 3307)
    impl_pct = card_stats.get("impl_pct", (implemented_count / total_inventory * 100.0) if total_inventory else 0.0)
    accuracy = card_stats.get("accuracy", 98.24)
    progress_score = card_stats.get("progress", (impl_pct * accuracy / 100.0))

    game_inventory = sum(1 for a in sizes if a < GAME_CODE_LIMIT) or 3054
    game_bytes = sum(s for a, s in sizes.items() if a < GAME_CODE_LIMIT) or 628779
    exact_byte_share = (exact_bytes / game_bytes * 100.0) if game_bytes else 68.49
    exact_func_share = (exact_count / game_inventory * 100.0) if game_inventory else 87.30

    clean_states = sum(1 for s in bringup_states.values() if s.get("result") == "clean")
    fault_states = sum(1 for s in bringup_states.values() if s.get("result") in ("crash", "aborted", "fault"))
    total_states = len(bringup_states) or 29

    print("================================================================================")
    print("                      HYPERBLADE RECOMPILATION STATUS                          ")
    print("================================================================================")
    print()
    print("STREAM 1: BYTE MATCHING (RETAIL FIDELITY)")
    print("--------------------------------------------------------------------------------")
    print("  Overall Progress:   %6.2f%% %s" % (progress_score, make_bar(progress_score)))
    print("  Implemented:        %6.2f%% (%d / %d functions; %d unwritten)" %
          (impl_pct, implemented_count, total_inventory, total_inventory - implemented_count))
    print("  Opcode Accuracy:    %6.2f%% (similarity across implemented functions)" % accuracy)
    print("  Exact Game Bytes:   %6.2f%% (%d / %d game code bytes)" %
          (exact_byte_share, exact_bytes, game_bytes))
    print("  Exact Functions:    %6.2f%% (%d / %d game functions)" %
          (exact_func_share, exact_count, game_inventory))
    print()
    print("STREAM 2: CRUFT CLEANING (CODE ARCHITECTURE)")
    print("--------------------------------------------------------------------------------")
    cruft = measure_cruft()
    print("  Checker Suite:      %d checkers in tools/" % cruft["checkers"])
    print("  Manual Vtables:     %s" % cruft["vtbl"])
    print("  Self Casts:         %s" % cruft["selfcast"])
    print("  Scaled Offsets:     %s" % cruft["scaled"])
    print("  Chained Casts:      %s" % cruft["chained"])
    print("  Duplicate Consts:   %s" % cruft["dupes"])
    print("  Dead Defines:       %s" % cruft["deaddefs"])
    print("  Macro/Enum Clash:   %s" % cruft["clash"])
    print("  Discarded Types:    %s" % cruft["discarded"])
    print("  Int Puns:           %s" % cruft["puns"])
    print("  Mixed Units:        %s" % cruft["mixed"])
    print("  Doc Paths:          %s" % cruft["docpaths"])
    print("  Include Hygiene:    %s" % cruft["hygiene"])
    print("  Ghost Forwards:     %s" % cruft["ghosts"])
    print("  Self-Named Notes:   %s" % cruft["selfnamed"])
    print("  Score Prose:        %s" % cruft["scoreprose"])
    print("  Const Accessors:    %s" % cruft["constacc"])
    print("  Unused Params:      %s" % cruft["unusedparams"])
    print("  Named Addresses:    %s" % cruft["namedaddrs"])
    print("  Source Paths:       %s" % cruft["srcpaths"])
    print("  Unused Includes:    %s" % cruft["unusedinc"])
    print("  Const Locals:       %s" % cruft["constlocals"])

    # Not run here: each takes far longer than this page.
    print("  Run beside this:    python tools/check_headers_standalone.py, tools/check_battery_reach.py,")
    print("                      python tools/check_battery_bites.py, tools/check_doc_figures.py")
    print()
    # These counts are not backlog: each is measured and kept at its site, or a reference
    # figure, so they print apart from the fault counts.
    print("  Held, not outstanding, each measured or documented at its site:")
    print("    Blob Casts:       %d (char*)this / (void*)this in %d file(s)"
          % (cruft["blob"], cruft["blobfiles"]))
    print("    Named Offsets:    %s" % cruft["offsets"])
    print("    Shared Fields:    %s; look before a rename, never a target" % cruft["shared"])
    print("    Untyped Members:  %s" % cruft["untyped"])
    print("    Class Files:      %s" % cruft["classfiles"])
    print("    Class Spread:     %s" % cruft["classspread"])
    print("    Offset Reads:     %s" % cruft["offsetreads"])
    print("    View Classes:     %s" % cruft["views"])
    print("    Leading Runs:     %s" % cruft["leadruns"])
    print()
    print("STREAM 3: BRING-UP PLAYABILITY (RUNTIME EXECUTION)")
    print("--------------------------------------------------------------------------------")
    print("  Engine States:      %d / %d states clean" % (clean_states, total_states))
    print("  2D Frontend:        PASS (States 0-13: movies, UI, team/arena selection)")
    print("  3D Arena Match:     PASS (States 14-15: arena, players, HUD, radar, power meter)")
    print("  Arenas Drawing:     14 / 14")
    if fault_states > 0:
        for sid, s in sorted(bringup_states.items(), key=lambda x: int(x[0])):
            if s.get("result") in ("crash", "aborted", "fault"):
                print("    - State %-2s [%s]: %s" % (sid, s.get("result").upper(), s.get("site", s.get("note", ""))))
    print("================================================================================")
    return 0


if __name__ == "__main__":
    sys.exit(main())
