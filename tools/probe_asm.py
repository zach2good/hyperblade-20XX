"""Compile one source file to a `/FAs` listing with the build's own flags.

From Git Bash, MSYS rewrites every `/flag` into a path, and cl then compiles at /Od and says
only `D4024 : unrecognized source file type`. So this passes every flag in the `-` spelling,
takes the flags from `build/CMakeFiles/hyperx_objects.dir/flags.make`, and refuses to answer
when every function in the listing comes back unoptimised.

    python tools/probe_asm.py src/engine/object.cpp
    python tools/probe_asm.py src/ui/screens/headtoheadsetupuiscreen.cpp --add -Og-
    python tools/probe_asm.py src/foo.cpp --func Widget__LoadArenaFrames

Exits 1 when the compile fails, when cl reports a mangled flag, or when every function in the
listing is unoptimised although the flags ask for optimisation. Exits 2 when there is no build directory
to read the flags from.
"""

import argparse
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FLAGS_FILE = os.path.join(ROOT, "build", "CMakeFiles", "hyperx_objects.dir", "flags.make")

# At /O2 VC6 omits the frame pointer in almost every function in this tree, so a unit where
# every function keeps one was compiled without the optimiser. A few optimised functions keep a
# frame, so one alone is only a note. A function that realigns its stack for a double is exempt.
UNOPTIMISED = re.compile(r"^\tmov\tebp, esp\n(?!\tand\tesp, -8)", re.MULTILINE)
BODY = re.compile(r"^(\S+)\s+PROC NEAR(.*?)^\1\s+ENDP", re.MULTILINE | re.DOTALL)


def read_flags(source):
    """The compiler, its include path and the flags this file is really built with."""
    if not os.path.exists(FLAGS_FILE):
        return None
    text = open(FLAGS_FILE, encoding="utf-8").read()
    compiler = re.search(r"# compile CXX with (.+)", text)
    includes = re.search(r"^CXX_INCLUDES = (.*)$", text, re.MULTILINE)
    flags = re.search(r"^CXX_FLAGS = (.*)$", text, re.MULTILINE)
    if not compiler or not flags:
        return None
    name = os.path.basename(source)
    custom = re.search(
        r"^# Custom flags: CMakeFiles/hyperx_objects\.dir/.*/%s\.obj_FLAGS = (.*)$"
        % re.escape(name),
        text,
        re.MULTILINE,
    )
    return (
        compiler.group(1).strip(),
        (includes.group(1).strip() if includes else ""),
        flags.group(1).strip() + (" " + custom.group(1).strip() if custom else ""),
    )


def dashed(argument):
    """`/O2` as `-O2`. MSYS rewrites the first spelling into a path and not the second."""
    if argument.startswith("/") and len(argument) > 1 and argument[1].isalpha():
        return "-" + argument[1:]
    return argument


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source")
    parser.add_argument("--add", nargs="*", default=[], help="extra flags, appended last")
    parser.add_argument("--func", default="", help="print only this function's body")
    parser.add_argument("--out", default="", help="where to write the listing")
    args = parser.parse_args()

    source = args.source.replace("/", os.sep)
    if not os.path.exists(os.path.join(ROOT, source)):
        print("no such source file: %s" % source)
        return 2

    found = read_flags(source)
    if found is None:
        print("no build/CMakeFiles/hyperx_objects.dir/flags.make; configure the build first")
        return 2
    compiler, includes, flags = found

    out = args.out or os.path.join(
        os.environ.get("TEMP", os.environ.get("TMP", ".")),
        os.path.basename(source) + ".asm",
    )
    command = [compiler]
    for argument in flags.split() + includes.split() + list(args.add):
        command.append(dashed(argument))
    command += ["-c", "-FAs", "-Fa" + out, "-Fo" + out + ".obj", source]

    done = subprocess.run(
        command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
    )
    if "D4024" in done.stdout or "D8021" in done.stdout:
        print("a flag reached cl as a file name, so the listing is not what you asked for:")
        print(done.stdout)
        return 1
    if done.returncode != 0 or not os.path.exists(out):
        print(done.stdout)
        return 1

    listing = open(out, encoding="utf-8", errors="replace").read()
    bodies = BODY.findall(listing)
    framed = [name for name, body in bodies if UNOPTIMISED.search(body)]
    optimised = any(f in " ".join(command) for f in ("-O2", "-O1", "-Ox", "-Og"))
    if optimised and "-Og-" not in command and framed:
        if len(framed) == len(bodies):
            print("every function has a frame pointer at %s, so the optimiser did not run" % flags)
            print("cl said: %s" % done.stdout.strip())
            return 1
        for name in framed:
            print("note: %s keeps a frame pointer; the rest of the unit is optimised" % name)

    warnings = [line for line in done.stdout.splitlines() if " warning C" in line]
    for line in warnings:
        print(line)
    print("listing: %s" % out)

    if args.func:
        body = re.search(
            r"^\S*%s\S*\s+PROC NEAR.*?^\S*%s\S*\s+ENDP" % (args.func, args.func),
            listing,
            re.MULTILINE | re.DOTALL,
        )
        if not body:
            print("no function matching %s in the listing" % args.func)
            return 1
        print(body.group(0))
    return 0


if __name__ == "__main__":
    sys.exit(main())
