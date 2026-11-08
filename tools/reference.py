"""Where the original executable lives.

The original is never committed, so its path is a local setting. It comes from the git-ignored
`reccmp-user.yml` that reccmp itself reads, or from an environment variable that overrides it.

    python tools/reference.py                     prints the v1.0.0.1 path, or explains why there is none
    python tools/reference.py --target HYPERX100  the same for v1.0.0.0

Exits 1 with a message on stderr when no copy is configured.
"""

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
USER_FILE = os.path.join(ROOT, "reccmp-user.yml")

MISSING = """no copy of the original executable is configured.

This build reads the resources out of your own copy of the game, because the repository
carries none of the original's content. Write %s:

targets:
  HYPERX:
    path: C:/path/to/your/HYPERX.EXE

The file is git-ignored. Setting HYPERX_REFERENCE in the environment overrides it."""


def reference_path(required=True, target="HYPERX"):
    from_env = os.environ.get("HYPERX_REFERENCE", "").strip()

    if from_env:
        return check(from_env, required)

    if not os.path.exists(USER_FILE):
        return fail(MISSING % USER_FILE, required)

    current = None

    for line in open(USER_FILE, encoding="utf-8"):
        stripped = line.strip()

        # A target is a two-space key under targets:, and its path sits under it.
        if line.startswith("  ") and not line.startswith("    ") and stripped.endswith(":"):
            current = stripped[:-1]
        elif current == target and stripped.startswith("path:"):
            return check(stripped.split("path:", 1)[1].strip(), required)

    return fail("%s names no path for target %s" % (USER_FILE, target), required)


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
    wanted = "HYPERX"

    if len(sys.argv) == 3 and sys.argv[1] == "--target":
        wanted = sys.argv[2]

    sys.stdout.write(reference_path(target=wanted) + "\n")
