"""Where the original executable lives.

The original is never committed, so its path is a local setting. It comes from the git-ignored
`hyperx-user.yml` at the repository root, or from an environment variable that overrides it.

    python tools/reference.py          prints the path, or explains why there is none

Exits 1 with a message on stderr when no copy is configured.
"""

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
USER_FILE = os.path.join(ROOT, "hyperx-user.yml")

MISSING = """no copy of the original executable is configured.

This build reads the resources out of your own copy of the game, because the repository
carries none of the original's content. Write %s:

path: C:/path/to/your/HYPERX.EXE

The file is git-ignored. Setting HYPERX_REFERENCE in the environment overrides it."""


def reference_path(required=True):
    from_env = os.environ.get("HYPERX_REFERENCE", "").strip()

    if from_env:
        return check(from_env, required)

    if not os.path.exists(USER_FILE):
        return fail(MISSING % USER_FILE, required)

    for line in open(USER_FILE, encoding="utf-8"):
        if "path:" in line:
            return check(line.split("path:", 1)[1].strip(), required)

    return fail("%s names no target path" % USER_FILE, required)


def check(path, required):
    if not os.path.isfile(path):
        return fail("the configured original does not exist: %s" % path, required)

    return path


def fail(message, required):
    if not required:
        return None

    sys.stderr.write(message + "\n")
    raise SystemExit(1)


if __name__ == "__main__":
    sys.stdout.write(reference_path() + "\n")
