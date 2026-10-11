"""Say what already claims an address, before a new function is written against it.

When a stub already stands at the address, a new function under a different name links beside
it, measures something plausible, and the callers keep calling the stub.

For each address this reports:

  * every marker in the tree at that address, with the declaration under it. The declaration is
    the part that matters: it names the class the callers already use, and a definition written
    under a different class name links beside the stub rather than replacing it.
  * Ghidra's name and signature, when the bridge is up.
  * the RET immediate, which is the argument count the declaration has to agree with.
  * who in the tree calls the name that declaration uses.

    python tools/claims.py 0x004183e0
    python tools/claims.py 0x004183e0 0x00418670 0x004189c0

The Ghidra bridge that tools/ghidra_api.py names is optional; without it the source half still reports.
"""
import glob
import re
import sys
import urllib.parse
import urllib.request

from ghidra_api import BASE as BRIDGE  # noqa: E402

MARKER = re.compile(r"^//\s*(STUB|FUNCTION|LIBRARY|VTABLE|GLOBAL|STRING):\s*HYPERX\s+"
                    r"(0x[0-9a-fA-F]+)\s*$")

# The word before the opening parenthesis, which is the name a caller has to write.
NAME = re.compile(r"([A-Za-z_~][\w:~]*)\s*\(")


def get(path, **params):
    url = "%s/%s?%s" % (BRIDGE, path, urllib.parse.urlencode(params))

    with urllib.request.urlopen(url, timeout=20) as answer:
        return answer.read().decode("utf-8", "replace")


def sources():
    return sorted(glob.glob("src/**/*.cpp", recursive=True) + glob.glob("src/**/*.h", recursive=True))


def findMarkers(address):
    """Every marker at the address, as (path, line, kind, declaration)."""
    found = []

    for path in sources():
        lines = open(path, encoding="utf-8").read().split("\n")

        for index, line in enumerate(lines):
            hit = MARKER.match(line.strip())

            if hit is None or int(hit.group(2), 16) != address:
                continue

            declaration = ""

            for step in range(6):
                if index + 1 + step >= len(lines):
                    break

                declaration += " " + lines[index + 1 + step].strip()

                if declaration.count("(") > 0 \
                        and declaration.count("(") == declaration.count(")"):
                    break

            found.append((path, index + 1, hit.group(1), declaration.strip()))

    return found


def bareName(declaration):
    """The name a caller writes, without its class."""
    hit = NAME.search(declaration)

    if hit is None:
        return None

    name = hit.group(1)

    if "::" in name:
        return name.split("::")[-1]

    return name


def callers(name, skip):
    """Files that mention the name, other than the ones that declare it."""
    found = []

    for path in sources():
        if path in skip:
            continue

        text = open(path, encoding="utf-8").read()

        if re.search(r"\b%s\b" % re.escape(name), text) is not None:
            found.append(path)

    return found


def retBytes(listing):
    for line in reversed(listing.strip().split("\n")):
        body = line.split(":", 1)[-1].split(";")[0].strip()

        if body.startswith("RET"):
            tail = body[3:].strip()

            if tail == "":
                return "none, so the caller cleans"

            return "%s bytes, so %d stack arguments" % (tail, int(tail, 16) // 4)

    return None


def report(address):
    print("0x%08x" % address)

    found = findMarkers(address)

    if not found:
        print("  nothing in the tree claims it")
    else:
        for path, line, kind, declaration in found:
            print("  %-8s %s:%d" % (kind, path, line))
            print("           %s" % declaration)

    try:
        info = get("get_function_by_address", address="0x%08x" % address)

        for line in info.strip().split("\n")[:2]:
            print("  ghidra   %s" % line.strip())
    except Exception:
        print("  ghidra   unavailable")

    try:
        listing = get("disassemble_function", address="0x%08x" % address)
        answer = retBytes(listing)

        if answer is not None:
            print("  pops     %s" % answer)
    except Exception:
        pass

    names = set()

    for path, line, kind, declaration in found:
        name = bareName(declaration)

        if name is not None:
            names.add(name)

    for name in sorted(names):
        skip = set(path for path, line, kind, declaration in found)
        users = callers(name, skip)

        if users:
            print("  callers  %s: %s" % (name, ", ".join(users)))
        else:
            print("  callers  %s: none" % name)

    print()


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2

    for argument in sys.argv[1:]:
        report(int(argument, 16))

    return 0


if __name__ == "__main__":
    sys.exit(main())
