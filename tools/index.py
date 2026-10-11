"""Write tools/README.md from the scripts themselves, and fail when it has gone stale.

The index is generated, so it cannot disagree with the directory.

    python tools/index.py            # rewrite tools/README.md
    python tools/index.py --check    # fail if it is not what the directory says

Every script's first line of prose is its entry, so the index is only as good as those
first lines. A script with no docstring and no leading comment is reported rather than
silently listed as blank: write one line saying what it does.

The battery runs `--check`, so adding a script and not regenerating fails before review.
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent
OUTPUT = ROOT / "README.md"

# The scripts the build and the score run, in the order a build reaches them.
CORE = ("cmake_build.py", "reference.py", "extract_resources.py", "stamp.py", "richcheck.py",
        "pecompare.py", "rsrccompare.py", "report.py", "progress.py", "check_regressions.py", "check_readme.py",
        "checkers.py", "check_battery_bites.py")

# Where the gated scripts are named. A script one of these runs is listed with the checkers.
GATES = ("../.github/workflows/ci.yml", "../CMakeLists.txt", "checkers.py")

# The families the other names fall into. Order is the order of the document, and anything
# that matches nothing lands in the last group.
FAMILIES = [
    ("check_", "Other checkers",
     "Checks you run by hand. Some need the game data, the Ghidra bridge or a bring-up run."),
    ("gen_", "Generators",
     "Write a file the tree carries from the reference image. A generated file is regenerated, "
     "never hand-edited."),
    ("find_", "Searches over the original",
     "Ask a question of the reference image and print what it finds."),
    ("name_", "Naming",
     "Propose names for what the original has and this tree does not. A proposal is not an "
     "answer: apply it, rescore, and drop what does not reach 100."),
    ("bringup_", "Bring-up",
     "Drive the bring-up build, which runs the game. Never measure that build."),
    ("fpo_", "The v1.0.0.0 frame-pointer tables",
     "v1.0.0.0 ships an unstripped debug directory. These read it and carry what it knows "
     "across to v1.0.0.1."),
    ("", "Everything else",
     "One-offs, readers and diagnostics. Most are run by hand with an address."),
]

FIRST_LINE = re.compile(r'\s*(?:"""|\'\'\'|#\s?)(.*)')


PREAMBLE = re.compile(r"(?:import\s|from\s|os\.chdir\(|os\.environ\[|sys\.path)")


def summary(path):
    """The script's first line of prose, or None when it has none.

    A script may open with its imports and put its description in a comment below them. The
    scan steps over import lines and the `os.` and `sys.path` setup, and stops at the first
    line that is neither those nor prose, so a comment above a function is never taken.

    A leading `Usage:` line is the invocation, so the line under it is taken instead.
    """
    block = []

    for line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        stripped = line.strip()

        if not stripped:
            if block:
                break

            continue

        if stripped.startswith(('"""', "'''")):
            match = FIRST_LINE.match(line)
            text = match.group(1).strip().rstrip('"').rstrip("'").strip()

            return text or None

        if stripped.startswith("#") and not stripped.startswith("#!"):
            block.append(stripped.lstrip("#").strip())

            continue

        if block or not PREAMBLE.match(stripped):
            break

    while block and block[0].lower().startswith("usage:"):
        block.pop(0)

    return block[0] if block else None


def rows():
    """Every script, its family and its first line, in the document's order."""
    scripts = sorted(p for p in ROOT.glob("*.py") if p.name != "index.py")
    core = [ROOT / name for name in CORE]
    placed = set(core)
    gates = "".join((ROOT / name).read_text(encoding="utf-8") for name in GATES)
    named = set(re.findall(r"(\w+\.py)\b", gates))
    named |= set(name + ".py" for name in re.findall(r"\b(\w+_\w+)\b", gates))
    gated = [path for path in scripts if path.name in named and path not in placed]
    placed.update(gated)
    grouped = [
        ("Build, score and gate", "What the build and the score run, in the order they run it.",
         core),
        ("Checkers", "Run by CI, by the `verify` build targets, or by `checkers.py`. Each fails "
         "on one fault and says which.", gated),
    ]

    for prefix, title, blurb in FAMILIES:
        members = []
        for path in scripts:
            if path in placed:
                continue
            if prefix and not path.name.startswith(prefix):
                continue
            members.append(path)
            placed.add(path)
        grouped.append((title, blurb, members))

    return grouped


def render():
    lines = [
        "# The tools",
        "",
        "**Generated by `python tools/index.py`. Do not edit by hand.** Each entry is the",
        "script's own first line, so a wrong entry is a wrong docstring and belongs in the",
        "script.",
        "",
        "`tools/requirements.txt` covers what these need. Anything reading the original resolves",
        "its path through `reference.py`, which reads the git-ignored `reccmp-user.yml`, so no",
        "script here carries anybody's disk layout.",
        "",
    ]

    missing = []

    for title, blurb, members in rows():
        if not members:
            continue
        lines.append("## %s" % title)
        lines.append("")
        lines.append(blurb)
        lines.append("")
        lines.append("| Script | What it does |")
        lines.append("|---|---|")
        for path in members:
            text = summary(path)
            if text is None:
                missing.append(path.name)
                text = "**no first line; add one**"
            lines.append("| `%s` | %s |" % (path.name, text.replace("|", "\\|")))
        lines.append("")

    return "\n".join(lines).rstrip() + "\n", missing


def main():
    text, missing = render()

    for name in missing:
        print("%s has no docstring and no leading comment" % name)

    if "--check" in sys.argv[1:]:
        current = OUTPUT.read_text(encoding="utf-8") if OUTPUT.exists() else ""
        if current.replace("\r\n", "\n") != text:
            print("tools/README.md is stale; run python tools/index.py")
            return 1
        print("tools/README.md matches the directory, %d script(s) indexed"
              % (text.count("\n| `")))
        return 1 if missing else 0

    OUTPUT.write_text(text, encoding="utf-8", newline="")
    print("wrote %s, %d script(s) indexed" % (OUTPUT, text.count("\n| `")))
    return 1 if missing else 0


if __name__ == "__main__":
    raise SystemExit(main())
