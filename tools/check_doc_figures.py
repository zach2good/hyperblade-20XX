# Check every number docs/RE_CRUFT.md puts in bold against the output of the tool that row names.
#
#     python tools/check_doc_figures.py           # exit 1 on a stale figure
#     python tools/check_doc_figures.py --list    # also list the figures counted by hand
#
# The test is blunt: every integer inside the row's `**bold**` must appear somewhere in the
# tool's output. A figure no tool produces must be registered in BY_HAND.
#
# It runs many tools, so it sits outside the battery. Run it after changing a figure and after
# any sweep that moves one. Read only; it exits 2 if a tool it ran changed the tree.

import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

DOC = os.path.join("docs", "RE_CRUFT.md")

ROW = re.compile(r"^\|\s*([^|]+?)\s*\|\s*\*\*(.+?)\*\*\s*\|(.*)\|\s*$", re.M)
TOOL = re.compile(r"python tools/([\w.]+\.py)((?:\s+--[\w-]+)*)")
NUMBER = re.compile(r"\b(\d+)\b")

# Tools that write to the tree, which this never runs. The list is the guard: a writer that is
# interrupted mid-edit leaves debris that no after-check can attribute. dirty() only catches a
# tool that completes and still leaves a mark.
WRITES = {
    "check_battery_bites.py": "plants a fault in a source file and restores it",
    "const_locals.py": "--apply rewrites a source file",
    "unused_includes.py": "--apply rewrites a source file",
    "index.py": "regenerates tools/README.md unless --check is passed",
}

# (row, figure) that no tool produces, and where the number comes from instead.
BY_HAND = {
}


def dirty():
    """What git says has changed, as one string to compare against itself."""
    run = subprocess.run(["git", "status", "--porcelain"],
                         capture_output=True, text=True, cwd=ROOT)

    return run.stdout


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv
    before = dirty()

    text = open(DOC, encoding="utf-8", errors="replace").read()

    checked = 0
    registered = 0
    faults = []
    skipped = []

    for hit in ROW.finditer(text):
        name = hit.group(1).strip()
        figure = hit.group(2)
        call = TOOL.search(hit.group(3))

        if not call:
            continue

        parts = [call.group(1)] + call.group(2).split()

        # Its own row would make it run itself.
        if parts[0] == os.path.basename(__file__):
            continue

        # A writing tool runs only with its read-only switch.
        if parts[0] in WRITES:
            if parts[0] == "index.py" and "--check" in parts:
                pass
            else:
                skipped.append((name, parts[0], WRITES[parts[0]]))
                continue

        run = subprocess.run([sys.executable, os.path.join("tools", parts[0])] + parts[1:],
                             capture_output=True, text=True, cwd=ROOT)
        out = run.stdout + run.stderr

        if dirty() != before:
            print("STOPPED  %s left a mark on the tree. Run `git status`, put back what it "
                  "changed, and add it to WRITES in this file." % " ".join(parts))

            return 2

        checked += 1

        for number in NUMBER.findall(figure):
            if re.search(r"\b%s\b" % number, out):
                continue

            if (name, number) in BY_HAND:
                registered += 1
                continue

            faults.append((name, figure, " ".join(parts), number))

    if show:
        for (name, number), why in sorted(BY_HAND.items()):
            print("BY HAND  %-40s %-6s %s" % (name[:40], number, why))

    for name, figure, call, number in faults:
        print("STALE    %-40s says %s, and %s does not answer %s"
              % (name[:40], figure[:34], call, number))

    for name, tool, why in skipped:
        print("NOT RUN  %-40s %-28s %s" % (name[:40], tool, why))

    print("%d row(s) name a tool this may run, %d figure(s) registered as counted by hand, "
          "%d stale, %d row(s) name a tool that writes and is not run"
          % (checked, registered, len(faults), len(skipped)))

    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
