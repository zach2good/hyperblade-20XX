"""Check every class against the slot arities its own table carries in the image.

Declaration order is slot order. For each table, symbols/vtablearity.h records how many bytes
each slot pops, read from the `ret N` of the real function. A class pinned to a table with
`// VTABLE: HYPERX 0x004bxxxx` or `// SLOT TABLE 0x004bxxxx` is checked slot by slot. SLOT
TABLE pins a class without asking reccmp to compare the table.

The slot list is built the way C++ builds it: the base's slots in order, then the derived
class's virtuals, each replacing the base slot of the same name or appending a new one. A
class with more slots than its table reports as an overrun, usually an override whose name no
longer matches the base's.

A slot is skipped, not guessed, when:

  - a by-value parameter has a type the tool cannot size;
  - the parameter list is `...`;
  - it lies past the last slot in the table that pops anything. The generator pads each table
    with pop-nothing placeholders, so a trailing `VP0` cannot be told from a real
    argumentless slot.

    python tools/check_slot_arity.py           report disagreements
    python tools/check_slot_arity.py --all     also list the slots that agree

Exit code 1 when a declared slot disagrees with the image.
"""

import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")
ARITY = os.path.join(ROOT, "symbols", "vtablearity.h")

MACRO = re.compile(r"^#define VTABLE_ARITY_(004b[0-9a-f]{4})\s*\{([^}]*)\}", re.M)
LENGTH = re.compile(r"^#define VTABLE_SLOTS_(004b[0-9a-f]{4}) (\d+)", re.M)
POP = re.compile(r"VP(\d+)")

# Either pin on the line above a class. Only a VTABLE marker adds a reccmp row.
MARKED = re.compile("// *(?:VTABLE: *HYPERX|SLOT TABLE) +(0x[0-9a-fA-F]+)[^" + chr(10) + "]*"
                    + chr(10) + "class ([0-9A-Za-z_]+)")

CLASS = re.compile(r"^(?:class|struct) (?:__declspec\(\w+\) )?([0-9A-Za-z_]+)(?: *: *public ([0-9A-Za-z_]+))? *"
                   + chr(10) + r"\{(.*?)" + chr(10) + r"\};", re.S | re.M)

VIRTUAL = re.compile(r"\bvirtual\s+([^;{]+?)\s*(?:=\s*0\s*)?[;{]", re.S)

# Comments are stripped from a class body, or the word "virtual" in prose reads as a slot.
# A "(*" opens a declarator, as in `float (*GetMatrixPtr())[4]`, not the parameter list.
NAMED = re.compile(r"([~]?[0-9A-Za-z_]+)\s*\((?!\*)")

# Everything that occupies exactly one dword when passed by value. Win32 handles and
# LP-prefixed types are pointers; the rest are the scalars the tree uses.
DWORD_TYPES = set("""
char short int long float bool signed unsigned void
BOOL BYTE WORD DWORD UINT INT LONG ULONG HRESULT
HDC HPALETTE HWND HBITMAP HINSTANCE HMENU HFONT HGDIOBJ HANDLE
WPARAM LPARAM COLORREF
""".split())

WIDE_TYPES = ("double", "__int64", "long long")

# Not part of the type. A __fastcall virtual takes its first declared argument in EDX.
CONVENTIONS = ("__fastcall", "__thiscall", "__stdcall", "__cdecl")


# A plain record of scalar fields passed by value occupies its own size on the stack.
RECORD = re.compile(r"^struct ([0-9A-Za-z_]+)" + chr(10) + r"\{" + chr(10)
                    + r"((?:    [0-9A-Za-z_]+ [0-9A-Za-z_]+;" + chr(10) + r")+)\};", re.M)
FIELD = re.compile(r"^    ([0-9A-Za-z_]+) [0-9A-Za-z_]+;$", re.M)

# An enum travels in one dword, as an argument and as a result.
ENUM = re.compile(r"^enum ([0-9A-Za-z_]+)$", re.M)

_records = None
_enums = None


def sourceNames(root):
    # Sources live in subdirectories, so walk rather than list. Names are relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def records():
    """{name: byte size} for every record declared as nothing but scalar fields."""
    global _records

    if _records is not None:
        return _records

    _records = {}

    for name in sorted(sourceNames(SRC)):
        if not name.endswith((".cpp", ".h")):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8",
                       errors="replace").read().replace(chr(13) + chr(10), chr(10))

        for match in RECORD.finditer(text):
            total = 0

            for kind in FIELD.findall(match.group(2)):
                if kind not in DWORD_TYPES:
                    total = None
                    break

                total += 8 if kind in WIDE_TYPES else 4

            if total:
                _records.setdefault(match.group(1), total)

    return _records


def enums():
    """The names of every enum declared in the tree."""
    global _enums

    if _enums is not None:
        return _enums

    _enums = set()

    for name, text in sources():
        _enums.update(ENUM.findall(text))

    return _enums


def isDword(word):
    return word in DWORD_TYPES or word in enums()


def slotBytes(parameter):
    """Stack bytes one written parameter occupies, or None when it cannot be sized."""
    text = " ".join(parameter.split())

    if text in ("", "void"):
        return 0

    if text == "...":
        return None

    if "*" in text or "&" in text:
        return 4

    for wide in WIDE_TYPES:
        if wide in text:
            return 8

    words = [word for word in re.split(r"[\s\[\]]+", text) if word]

    # Any word that is not a scalar keyword, and does not follow one as the parameter's name,
    # is a by-value class: sized when it is a known record, unsizable otherwise.
    for index, word in enumerate(words):
        if isDword(word):
            continue

        if index > 0 and isDword(words[index - 1]):
            continue

        if word in records():
            return records()[word]

        return None

    return 4


def parameters(declaration):
    named = NAMED.search(declaration)

    if not named:
        return None

    open_at = named.end() - 1

    depth = 0

    for index in range(open_at, len(declaration)):
        if declaration[index] == "(":
            depth += 1
        elif declaration[index] == ")":
            depth -= 1

            if depth == 0:
                inner = declaration[open_at + 1:index]
                break
    else:
        return None

    if not inner.strip():
        return []

    parts = []
    depth = 0
    current = ""

    for character in inner:
        if character in "(<[":
            depth += 1
        elif character in ")>]":
            depth -= 1

        if character == "," and depth == 0:
            parts.append(current)
            current = ""
        else:
            current += character

    parts.append(current)

    return parts


def returnBytes(declaration):
    """4 when VC6 gives the call a hidden result pointer, 0 otherwise.

    A result that is not 1, 2 or 4 bytes comes back through a caller-supplied pointer that the
    callee pops, so `ret N` counts it: `Point GetPosition()` pops four.
    """
    head = declaration[:declaration.index("(")]

    # A pointer or reference result comes back in EAX, whatever it points to.
    if "*" in head or "&" in head:
        return 0

    words = [word for word in re.split(r"[\s*&]+", head) if word]

    if len(words) < 2:
        return 0

    for word in words[:-1]:
        if word in CONVENTIONS:
            continue

        if not isDword(word) and not any(wide in word for wide in WIDE_TYPES):
            return 4

    return 0


def declaredBytes(declaration):
    parts = parameters(declaration)

    if parts is None:
        return None

    total = returnBytes(declaration)

    # The first argument of a __fastcall virtual rides in EDX, so the slot never pops it.
    if any(convention in declaration for convention in CONVENTIONS[:1]) and parts:
        parts = parts[1:]

    for part in parts:
        width = slotBytes(part)

        if width is None:
            return None

        total += width

    return total


def sources():
    for name in sorted(sourceNames(SRC)):
        if name.endswith((".cpp", ".h")):
            path = os.path.join(SRC, name)
            text = open(path, encoding="utf-8", errors="replace").read()

            yield name, text.replace(chr(13) + chr(10), chr(10))


def arities():
    found = {}

    for address, body in MACRO.findall(open(ARITY, encoding="utf-8").read()):
        found[int(address, 16)] = [int(width) for width in POP.findall(body)]

    return found


def lengths():
    """{address: how many slots the table really has, before the generator's padding}."""
    found = {}

    for address, count in LENGTH.findall(open(ARITY, encoding="utf-8").read()):
        found[int(address, 16)] = int(count)

    return found


def classes():
    """{name: (base, [(slot name, declared bytes or None)])} for every class in src/."""
    found = {}

    for name, text in sources():
        for match in CLASS.finditer(text):
            if match.group(1) in found:
                continue

            slots = []

            body = code(match.group(3))

            for declaration in VIRTUAL.findall(body):
                declaration = " ".join(declaration.split())
                hit = NAMED.search(declaration)

                if not hit:
                    continue

                # The scalar deleting destructor takes the flags word.
                if hit.group(1).startswith("~"):
                    slots.append(("~", 4))
                    continue

                slots.append((hit.group(1), declaredBytes(declaration)))

            found[match.group(1)] = (match.group(2), slots)

    return found


def marked():
    found = {}

    for name, text in sources():
        for address, klass in MARKED.findall(text):
            found.setdefault(klass, (int(address, 16), name))

    return found


def layout(klass, known, seen=None):
    """The class's whole slot list, base first, an override replacing the slot it names."""
    if seen is None:
        seen = set()

    if klass not in known or klass in seen:
        return []

    seen.add(klass)
    base, slots = known[klass]
    table = layout(base, known, seen) if base else []

    for name, width in slots:
        # Many bases name slot zero ScalarDelete, which is the scalar deleting destructor, so a
        # derived `virtual ~X()` overrides it rather than adding a slot.
        wanted = ("~", "ScalarDelete") if name == "~" else (name,)

        for index, (existing, unused) in enumerate(table):
            if existing in wanted:
                table[index] = (existing, width)
                break
        else:
            table.append((name, width))

    return table


def main():
    showAll = "--all" in sys.argv[1:]

    widths = arities()
    counts = lengths()
    known = classes()
    pinned = marked()

    faults = []
    overruns = []
    agreed = 0
    skipped = 0

    for klass in sorted(pinned):
        address, home = pinned[klass]

        if address not in widths:
            continue

        table = layout(klass, known)

        # A class pinned to a table cannot have more slots than the table does.
        if address in counts and len(table) > counts[address]:
            overruns.append((klass, home, address, len(table), counts[address],
                             [name for name, unused in table[counts[address]:]]))

        # The last slot in the table that pops anything. Past it the padding and a real
        # argumentless slot look the same.
        bound = -1

        for index, width in enumerate(widths[address]):
            if width:
                bound = index

        for index, (name, width) in enumerate(table):
            if index > bound:
                break

            if width is None:
                skipped += 1
                continue

            if width == widths[address][index]:
                agreed += 1

                if showAll:
                    print("agree    %s::%s slot %d, %d bytes" % (klass, name, index, width))

                continue

            faults.append((klass, home, address, index, name, width,
                           widths[address][index]))

    for klass, home, address, index, name, width, actual in faults:
        print("%s::%s  slot %d of 0x%08x declares %d bytes, the image pops %d  (%s)"
              % (klass, name, index, address, width, actual, home))

    for klass, home, address, declared, real, extra in overruns:
        print("%s declares %d slots where 0x%08x has %d: %s  (%s)"
              % (klass, declared, address, real, " ".join(extra), home))

    print("%d class(es) carry a table this can read; %d slot(s) agree, %d disagree, "
          "%d unsizable, %d overrun" % (len(pinned), agreed, len(faults), skipped,
                                        len(overruns)))

    if faults:
        print("Declaration order is slot order. Move the declaration, do not change the type.")

    if overruns:
        print("A slot past the end of the table is an override whose name stopped matching "
              "the base's.")

    if faults or overruns:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
