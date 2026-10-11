"""Measure how much one reference weighs in VC6's callee-saved register ranking.

Each template is a function body. It must define `x` before `y`, keep both live across a call,
and say where their references go: `$X` expands to n copies of `f(x);` and `$Y` to K copies of
`f(y);`. The script compiles the body for n = 1, 2, ... with the build's flags and prints the
smallest n at which `x` takes ESI. `x` wins a tie because it is defined first, so n/K is the
weight of a `$Y` reference over an `$X` reference. docs/MATCHING.md (The weight of a reference)
records what the probes found.

    python tools/alloc_weight.py 'int x = f(a); int y = f(b); $X if (g) { f(1); } $Y'
    python tools/alloc_weight.py --k 30 'int x = f(a); int y = f(b); $X do { $Y } while (--g);'

The body sees `int f(int)`, the globals `g`, `h`, `k`, `l` and the parameters `a` to `e`.
Exits 1 when a compile fails, 2 when there is no build directory to read the compiler from.
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile

from probe_asm import ROOT, dashed, read_flags

PREAMBLE = "int f(int);\nextern int g, h, k, l;\n"
BODY = re.compile(r"^\?s(\d+)@\S*\s+PROC NEAR(.*?)ENDP", re.MULTILINE | re.DOTALL)
FIRST_COPY = re.compile(r"\tmov\t(e\w\w), eax")
TEMPLATE_FLAGS = ["-Ox", "-Gf"]


def first_win(compiler, flags, template, count, ceiling):
    """The smallest n at which x is copied into ESI, or None below the ceiling."""
    functions = [PREAMBLE]
    for n in range(1, ceiling):
        body = template.replace("$X", "f(x); " * n).replace("$Y", "f(y); " * count)
        functions.append("int s%d(int a, int b, int c, int d, int e) { %s return 0; }" % (n, body))
    with tempfile.TemporaryDirectory() as scratch:
        source = os.path.join(scratch, "alloc_weight.cpp")
        listing = source + ".asm"
        with open(source, "w") as handle:
            handle.write("\n".join(functions))
        command = [compiler] + [dashed(a) for a in flags.split()] + TEMPLATE_FLAGS
        command += ["-c", "-FAs", "-Fa" + listing, "-Fo" + source + ".obj", source]
        done = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, text=True)
        if done.returncode != 0 or not os.path.exists(listing):
            print(done.stdout)
            return -1
        text = open(listing, errors="replace").read()
    wins = []
    for number, body in BODY.findall(text):
        # The first callee-saved copy of a call result is x's register.
        copy = FIRST_COPY.search(body)
        if copy and copy.group(1) == "esi":
            wins.append(int(number))
    if wins:
        return min(wins)
    return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("template", nargs="+")
    parser.add_argument("--k", type=int, default=60, help="references of y per probe")
    args = parser.parse_args()

    found = read_flags("alloc_weight.cpp")
    if found is None:
        print("no build/CMakeFiles/hyperx_objects.dir/flags.make; configure the build first")
        return 2
    compiler, _, flags = found

    status = 0
    for template in args.template:
        n = first_win(compiler, flags, template, args.k, args.k * 8 + 2)
        if n == -1:
            status = 1
            continue
        if n is None:
            print("x never wins below n=%d :: %s" % (args.k * 8 + 2, template))
            continue
        print("n=%d, y/x %.3f :: %s" % (n, float(n) / args.k, template))
    return status


if __name__ == "__main__":
    sys.exit(main())
