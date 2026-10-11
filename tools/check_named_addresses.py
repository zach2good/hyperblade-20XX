# Find comments that name a function at an address where the FUNCTION marker names another.
#
#     python tools/check_named_addresses.py           # exit 1 on any disagreement
#     python tools/check_named_addresses.py --list    # also confirm when all agree
#
# A disagreement is usually an old spelling from before a body became a member, or a body that
# was re-read and renamed since the comment was written.
#
# A sentence that says the name is historical ("it was", "stood as" and similar) is not a fault.
# An address with no marker is skipped. Only src/ is read.
#
# Read only.

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

MARK = re.compile(r"^// FUNCTION: HYPERX (0x[0-9a-f]+)\r?\n(?://[^\r\n]*\r?\n)*"
                  r"[^\r\n(]*?\b([A-Za-z_]\w*(?:::~?\w+)?)\s*\(", re.M)

CLAIM = re.compile(r"\b([A-Z]\w*(?:::~?\w+)?)\s+at\s+(0x[0-9a-fA-F]{6,8})\b")

# The tree's own way of saying a name is historical.
WAS = ("it was ", "stood as", "used to be", "was called", "had been", "retired",
       "before it", "old name", "was named")

SKIP = ("src/jpeg/",)


def sources(suffix):
    for directory, unused, names in os.walk(os.path.join(ROOT, "src")):
        for name in sorted(names):
            if not name.endswith(suffix):
                continue

            path = os.path.join(directory, name)
            shown = os.path.relpath(path, ROOT).replace(os.sep, "/")

            if not shown.startswith(SKIP):
                yield path, shown


def owners():
    """The body each marked address belongs to."""
    found = {}

    for path, shown in sources(".cpp"):
        text = open(path, encoding="utf-8", errors="replace").read()

        for address, who in MARK.findall(text):
            found[address.lower()] = who

    return found


def agrees(claimed, real):
    return claimed == real or real.endswith("::" + claimed) or claimed.endswith("::" + real)


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    owner = owners()
    faults = []
    claims = 0

    for suffix in (".cpp", ".h"):
        for path, shown in sources(suffix):
            lines = open(path, encoding="utf-8", errors="replace").read().splitlines()

            for number, line in enumerate(lines, 1):
                at = line.find("//")

                if at < 0:
                    continue

                body = line[at:]

                for claimed, address in CLAIM.findall(body):
                    real = owner.get(address.lower())

                    if real is None:
                        continue

                    claims += 1

                    if agrees(claimed, real):
                        continue

                    if any(one in body.lower() for one in WAS):
                        continue

                    faults.append((shown, number, claimed, address, real))

    for shown, number, claimed, address, real in faults:
        print("%-44s says %s at %s, the marker says %s"
              % ("%s:%d" % (shown, number), claimed, address, real))

    if show and not faults:
        print("every one of the %d claim(s) agrees with its marker" % claims)

    print("%d comment(s) name a function and a marked address, %d disagreeing with the marker"
          % (claims, len(faults)))

    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
