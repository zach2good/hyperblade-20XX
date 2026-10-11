"""Read how many bytes the client allocates for each class, and say which of ours disagree.

`new X` in the original is a `push <size>`, a call to operator new and a call to X's
constructor, so the image records each class's size and whose it is. This is the only check that
catches a class of the wrong length.

Declared sizes come from the header: the last field's offset plus its width, rounded up to the
alignment of the widest member. A class with a field it cannot size is reported as unknown.

    python tools/object_sizes.py            # every class with a size in the image
    python tools/object_sizes.py --diff     # only the ones that disagree

To pin a result, write `STATIC_ASSERT(sizeof(X) == N);` in src/common/classsizes.cpp.
The build then fails when the layout drifts.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import image_tables as g
import slot_owners as s

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

# operator new. Every `new X` in the image goes through it.
OPERATOR_NEW = 0x0049f2bb

# How far past the allocation to look for the constructor call.
REACH = 0x60


def sourceNames(root):
    # Paths come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def allocations(image):
    """{class: the sizes the client pushes before its constructor}."""
    known = s.owners()
    start, size = image.text[1], image.text[2]
    at = image.offset(start)
    sites = []
    pending = None

    for instruction in image.md.disasm(image.data[at:at + size], start):
        if instruction.mnemonic == "push":
            try:
                pending = (instruction.address, int(instruction.op_str, 16))
            except ValueError:
                pending = None

            continue

        if instruction.mnemonic == "call" and pending is not None:
            try:
                target = int(instruction.op_str, 16)
            except ValueError:
                target = None

            if target == OPERATOR_NEW:
                sites.append(pending)

        pending = None

    found = {}

    for address, value in sites:
        look = image.offset(address)

        for instruction in image.md.disasm(image.data[look:look + REACH], address):
            if instruction.mnemonic != "call":
                continue

            try:
                target = int(instruction.op_str, 16)
            except ValueError:
                continue

            if target == OPERATOR_NEW:
                continue

            who = known.get(target)

            if who is None:
                break

            # A constructor is Klass::Klass. The tilde is not optional: a deleting destructor
            # after an allocation would otherwise take that allocation's size.
            hit = re.search(r"([0-9A-Za-z_]+)::([0-9A-Za-z_]+)\s*\(", who[1])

            if hit and hit.group(1) == hit.group(2):
                found.setdefault(hit.group(1), set()).add(value)

            break

    return found


BODY = re.compile(r"// FUNCTION: HYPERX (0x00[0-9a-fA-F]{6})" + chr(10)
                  + r"([^" + chr(10) + r"]*" + chr(10) + r"?[^{]*)" + chr(92) + r"{")


def ordered(image):
    """{class: sizes} from functions whose `new` count matches the client's allocation count.

    The pairing is positional, so it holds only where the two counts agree. It reaches classes
    whose constructor is not the first call after the allocation.
    """
    found = {}

    for name in sorted(sourceNames(SRC)):
        if not name.endswith(".cpp"):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8",
                       errors="replace").read().replace(chr(13) + chr(10), chr(10))

        for hit in BODY.finditer(text):
            end = text.find(chr(10) + "}" + chr(10), hit.end())
            body = text[hit.end():end if end > 0 else len(text)]
            news = re.findall(r"new ([A-Z][0-9A-Za-z_]*)" + chr(92) + r"s*[;(" + chr(92) + r"[]", body)

            if not news:
                continue

            address = int(hit.group(1), 16)
            at = image.offset(address)

            if at is None:
                continue

            pending = None
            sizes = []

            for instruction in image.md.disasm(image.data[at:at + 0x1200], address):
                if instruction.mnemonic == "ret":
                    break

                if instruction.mnemonic == "push":
                    try:
                        pending = int(instruction.op_str, 16)
                    except ValueError:
                        pending = None

                    continue

                if instruction.mnemonic == "call" and pending is not None:
                    try:
                        target = int(instruction.op_str, 16)
                    except ValueError:
                        target = None

                    if target == OPERATOR_NEW:
                        sizes.append(pending)

                pending = None

            if len(sizes) != len(news):
                continue

            for klass, size in zip(news, sizes):
                found.setdefault(klass, set()).add(size)

    return found


CLASS = re.compile(r"^(?:class|struct) ([0-9A-Za-z_]+)(?: *: *public ([0-9A-Za-z_]+))? *"
                   + chr(10) + r"\s*\{(.*?)" + chr(10) + r"\};", re.S | re.M)
FIELD = re.compile(r"^\s+((?:[\w:*&<>]+\s+)*[\w:*&<>]+?)[\s*]([0-9A-Za-z_]+)"
                   r"(\[([^\]]*)\])?;", re.M)

WIDTH = {"char": 1, "unsigned char": 1, "signed char": 1, "short": 2, "unsigned short": 2,
         "int": 4, "unsigned int": 4, "long": 4, "unsigned long": 4, "float": 4,
         "DWORD": 4, "UINT": 4, "HRESULT": 4, "HDC": 4, "HPALETTE": 4, "HGDIOBJ": 4}


def width(kind, extent):
    """The declared width in bytes, or None when this cannot size it."""
    kind = " ".join(kind.split())

    if "*" in kind:
        return 4

    one = WIDTH.get(kind)

    if one is None:
        return None

    if extent is None:
        return one

    try:
        return one * int(eval(extent, {"__builtins__": {}}, {}))
    except Exception:
        return None


def main():
    onlyNew = "--new" in sys.argv[1:]
    image = g.Image(g.referencePath())
    client = allocations(image)
    client.update(ordered(image))

    pinned = set()

    for name in sorted(sourceNames(SRC)):
        if not name.endswith(".h"):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8", errors="replace").read()

        for hit in re.findall(r"typedef char Probe_([0-9A-Za-z_]+)\[", text):
            pinned.add(hit)

    rows = [(k, sorted(v)) for k, v in client.items() if len(v) == 1]
    loose = [(k, v) for k, v in rows if k not in pinned]

    print("%d class(es) have one size in the image; %d of them already carry a Probe_"
          % (len(rows), len(rows) - len(loose)))

    if onlyNew:
        print("")

        for klass, sizes in sorted(loose):
            print("typedef char Probe_%s[sizeof(%s) == 0x%x ? 1 : -1];"
                  % (klass, klass, sizes[0]))

        return 0

    print("")

    for klass, sizes in sorted(rows):
        print("    %-30s client 0x%-8x %s"
              % (klass, sizes[0], "" if klass in pinned else "NOT PINNED"))

    return 0


if __name__ == "__main__":
    sys.exit(main())
