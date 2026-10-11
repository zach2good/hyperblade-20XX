"""Run reccmp's annotation linter and fail on everything except the one rule this tree rejects.

The tree cannot obey `function_out_of_order`: VC6 tail-merges every function of a translation
unit except the first it optimises, so position in the file changes codegen. That rule is an
error in reccmp, so `--no-warnfail` does not reach it. This runs the linter, counts that rule,
and fails on any other line.

    python tools/check_lint.py            # fail on any fault but the ordering rule
    python tools/check_lint.py --all      # print the ordering reports too

If reccmp ever grows a way to disable one rule, delete this and pass that flag instead.

There is no list of accepted faults: the tree is at zero, so any fault here is new. A by-name
marker belongs in the class's header, not in a .cpp.
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
IGNORED = "function_out_of_order"

# The linter colours its output, so the rule name has escape codes around it.
COLOUR = re.compile(r"\x1b\[[0-9;]*m")
FAULT = re.compile(r"(error|warning):\s*(?:\[[^\]]*\]\s*)?([a-z_]+)")


def main():
    show_all = "--all" in sys.argv[1:]

    result = subprocess.run(
        [sys.executable, "-m", "reccmp.tools.decomplint", "--target", "HYPERX", "src"],
        cwd=str(ROOT), capture_output=True, text=True)

    lines = COLOUR.sub("", result.stdout + result.stderr).splitlines()

    ignored = 0
    other = []
    context = ""

    for line in lines:
        if not line.strip():
            continue

        # The linter prints the file on its own line, then each fault indented beneath it.
        if not line.startswith(" "):
            context = line.strip()
            continue

        match = FAULT.search(line)
        if not match:
            continue

        if match.group(2) == IGNORED:
            ignored += 1
            if show_all:
                print("  ordering  %s %s" % (context, line.strip()))
            continue

        other.append("%s %s" % (context, line.strip()))

    for fault in other:
        print(fault)

    print("%d %s reported and ignored, %d other fault(s)" % (ignored, IGNORED, len(other)))

    if ignored == 0:
        print("Nothing is out of order any more. Delete this wrapper and run the linter "
              "directly with --warnfail.")

    return 1 if other else 0


if __name__ == "__main__":
    raise SystemExit(main())
