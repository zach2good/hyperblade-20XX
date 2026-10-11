"""Push everything this repository knows into your own Ghidra database.

A Ghidra database holds the program's bytes, so the repository ships none. Import your own
copy of `HYPERX.EXE` into your own project, then run this. reccmp reads the PDB of your build,
pairs it against your copy through the markers in `src/`, and writes function names and
signatures, globals, vtables, and struct and class types into your database.

    python tools/publish_ghidra.py --project C:/work/hyperx.gpr --file /HYPERX.EXE
        [--ghidra DIR] [--refuse NAME_OR_DIR ...] [--dry-run]

Ghidra must be closed: the headless import takes the project lock. Needs a finished
`cmake --build build`.

`--refuse` names a project, by its name or its directory, that must never be written: a database
you keep by hand, say. That check runs first, before anything else can fail.
"""
import argparse
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def parse():
    parser = argparse.ArgumentParser(description="Publish this project's names into a Ghidra database.")
    parser.add_argument("--project", required=True, metavar="<path.gpr>",
                        help="your Ghidra project file, e.g. C:/work/hyperx.gpr")
    parser.add_argument("--file", required=True, metavar="<program>",
                        help="the program inside that project, e.g. /HYPERX.EXE")
    parser.add_argument("--ghidra", metavar="<dir>",
                        help="the Ghidra installation; defaults to GHIDRA_INSTALL_DIR")
    parser.add_argument("--refuse", action="append", default=[], metavar="<name or dir>",
                        help="a Ghidra project this must never write to; repeatable")
    parser.add_argument("--dry-run", action="store_true",
                        help="run every check and print the command, without starting Ghidra")
    return parser.parse_args()


def ghidraInstall(named):
    """The Ghidra directory, checked here because a wrong one fails deep inside the JVM."""
    found = named or os.environ.get("GHIDRA_INSTALL_DIR")

    if not found:
        raise SystemExit("set GHIDRA_INSTALL_DIR, or pass --ghidra <dir>, naming your Ghidra installation")

    path = Path(found)

    if not (path / "ghidraRun.bat").exists() and not (path / "ghidraRun").exists():
        raise SystemExit("%s holds no ghidraRun; that is not a Ghidra installation" % path)

    return path


def project(named, refused):
    """The project directory and name, refusing any project named in refused."""
    path = Path(named).resolve()

    if path.suffix.lower() != ".gpr":
        raise SystemExit("--project wants the .gpr file itself, not %s" % path)

    directory = path.parent
    name = path.stem

    if name in refused or any(Path(entry).resolve() == directory for entry in refused):
        raise SystemExit("refusing to write to %s, which --refuse names" % path)

    if not path.exists():
        raise SystemExit("no Ghidra project at %s; create one and import your copy of HYPERX.EXE first" % path)

    return directory, name


def program(named):
    """The program's path inside the Ghidra project.

    Git Bash rewrites `/HYPERX.EXE` into a Windows path, so a path that looks rewritten is
    refused with advice.
    """
    if "\\" in named or (len(named) > 1 and named[1] == ":"):
        raise SystemExit("\n".join([
            "--file wants the path inside the Ghidra project, such as /HYPERX.EXE, and this one",
            "arrived as %s." % named,
            "Git Bash rewrote it. Run with MSYS_NO_PATHCONV=1, double the leading slash as",
            "//HYPERX.EXE, or use cmd or PowerShell."]))

    if not named.startswith("/"):
        return "/" + named

    return named


def build():
    """The symbol build reccmp reads, or a message naming the step that makes it."""
    try:
        from reccmp.project.detect import RecCmpProject
    except ImportError:
        raise SystemExit("reccmp is not installed; pip install reccmp")

    found = RecCmpProject.from_directory(ROOT)

    if not found:
        raise SystemExit("no reccmp project at %s" % ROOT)

    target = found.get("HYPERX")

    if not Path(target.original_path).is_file():
        raise SystemExit("your copy of the original is not at %s; name it in reccmp-user.yml"
                         % target.original_path)

    for path in (target.recompiled_path, target.recompiled_pdb):
        if not Path(path).is_file():
            raise SystemExit("%s is missing; run cmake --build build first" % path)

    return target


def main():
    args = parse()

    # The refusal is checked first, before anything that could fail for an ordinary reason.
    directory, name = project(args.project, args.refuse)
    inside = program(args.file)
    install = ghidraInstall(args.ghidra)
    target = build()

    command = [sys.executable, "-m", "reccmp.ghidra.ghidra_import",
               "--target", "HYPERX",
               "--local-project-dir", str(directory),
               "--local-project-name", name,
               "--file", inside]

    print("Ghidra      : %s" % install)
    print("Project     : %s in %s" % (name, directory))
    print("Program     : %s" % inside)
    print("Reading     : %s" % target.recompiled_pdb)
    print("Comparing   : %s" % target.original_path)
    print()

    if args.dry_run:
        print("dry run, not starting Ghidra:")
        print("  " + " ".join(command))
        return 0

    environment = dict(os.environ)
    environment["GHIDRA_INSTALL_DIR"] = str(install)

    print("Close Ghidra before this runs: the import takes the project lock.")
    return subprocess.call(command, cwd=str(ROOT), env=environment)


if __name__ == "__main__":
    raise SystemExit(main())
