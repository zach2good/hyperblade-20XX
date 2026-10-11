"""Find classes that are the same class declared twice.

A view is a class that names a few virtual slots and pads the rest with SlotNN. This pairs views
by the evidence they carry: slot index, arity and return width.

Two classes pair when every slot they both name agrees on arity and at least two of those also
agree on return width. Slot zero and SlotNN padding are ignored. Slots that agree on arity but
not width are printed under the pair as the part still to settle. The output is ranked pairs,
not families, because a transitive closure of two-slot links joins unrelated classes.

A pair where both sides are decoded (a VTABLE, SLOT TABLE or COM VTABLE marker, or part of a
`class A : public B` line) is dropped. A pair with one decoded side is printed with a star.

    python tools/view_classes.py              # candidate duplicates, strongest first
    python tools/view_classes.py --all        # every view class and its slots
    python tools/view_classes.py --twice      # classes declared in more than one file

--twice lists every class declared in more than one file and exits 1 if there is any. Nothing
else catches these while the two declarations never meet in one translation unit, and they can
disagree about the base or the layout.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

# `class` or `struct`, with `public:` optional.
CLASS = re.compile(r"^(?:class|struct) (\w+)(?: : public \w+)?\n\{\n(?:public:\n)?(.*?)^\};",
                   re.S | re.M)
# pooldtors.h declares many pool owners with one slot signature on purpose, so it is skipped.
GENERATED = ("pooldtors.h",)

# A virtual with an inline body is still a slot. Dropping it would shift every later index.
SLOT = re.compile(r"^\s+virtual\s+(.+?)\s*(?:=\s*0\s*)?(?:;|\{)", re.M)
NAME = re.compile(r"\b(\w+)\s*\(")


def sourceNames(root):
    # Paths come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def arity(declaration):
    inner = declaration[declaration.index("(") + 1:declaration.rindex(")")].strip()
    if inner in ("", "void"):
        return 0
    depth = 0
    count = 1
    for character in inner:
        if character in "(<[":
            depth += 1
        elif character in ")>]":
            depth -= 1
        elif character == "," and depth == 0:
            count += 1
    return count


def returns(declaration):
    """The return type, normalised. Pointers collapse to one class, because a view calls
    everything it has not decoded `void*`.
    """
    head = declaration[:declaration.index("(")].strip()
    head = head.rsplit(" ", 1)[0].strip() if " " in head else head

    if head.endswith("*") or head.startswith("class ") or head.startswith("struct "):
        return "pointer"

    return head.replace("unsigned ", "u")


def views():
    found = {}
    for name in sorted(sourceNames(SRC)):
        if not name.endswith((".cpp", ".h")) or name in GENERATED:
            continue
        path = os.path.join(SRC, name)
        text = open(path, encoding="utf-8", errors="replace").read().replace("\r\n", "\n")
        for match in CLASS.finditer(text):
            slots = {}
            for index, declaration in enumerate(SLOT.findall(match.group(2))):
                if "(" not in declaration or ")" not in declaration:
                    continue
                hit = NAME.search(declaration)
                if hit:
                    slots[index] = (hit.group(1), arity(declaration), returns(declaration))
            if len(slots) >= 2:
                found[match.group(1)] = (name, slots)
    return found


# Every way a class says it is not a stand-in: reccmp's marker, the slot-table pin that
# tools/check_slot_arity.py reads, and a COM interface's mark. Both patterns take `class` or
# `struct`.
DECODED_MARKER = re.compile("// *(?:VTABLE: *HYPERX +0x[0-9a-fA-F]+|SLOT TABLE +0x[0-9a-fA-F]+"
                            + "|COM VTABLE)[^" + chr(10) + "]*"
                            + chr(10) + "(?:class|struct) ([0-9A-Za-z_]+)(?![0-9A-Za-z_])")
BASE_OF = re.compile("^(?:class|struct) ([0-9A-Za-z_]+) : public ([0-9A-Za-z_]+)", re.M)


def decodedClasses():
    """Classes the tree has already pinned to a table, and the bases they stand on.

    Only pairs where both sides are decoded are dropped. Two decoded classes can share slots
    because one derives from the other.
    """
    found = set()

    for name in sorted(sourceNames(SRC)):
        if not name.endswith((".cpp", ".h")):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8", errors="replace").read()
        text = text.replace(chr(13) + chr(10), chr(10))

        found.update(DECODED_MARKER.findall(text))

        # Both halves of every `class A : public B` line. A view is a stand-in with no
        # hierarchy; a class that has a base or is one has already been placed.
        for derived, base in BASE_OF.findall(text):
            found.add(derived)
            found.add(base)

    return found


def named(slots):
    """Slots with a decoded name, except slot zero.

    Slot zero is the deleting destructor on every class in the image, so counting it makes
    every pair of views look related and the families chain into one.
    """
    return {i for i, entry in slots.items()
            if i != 0 and not re.fullmatch(r"Slot\d+", entry[0])}


DECLARED = re.compile(r"^(?:class|struct) (\w+)(?: : public (\w+))?" + chr(10)
                      + r"\{", re.M)


def twice():
    """Every class declared in more than one file, with the files and the bases."""
    where = {}

    for name in sorted(sourceNames(SRC)):
        if not name.endswith((".cpp", ".h")) or name in GENERATED:
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8", errors="replace").read()
        text = text.replace(chr(13) + chr(10), chr(10))

        for match in DECLARED.finditer(text):
            where.setdefault(match.group(1), []).append((name, match.group(2)))

    return dict((k, v) for k, v in where.items() if len(v) > 1)


def report_twice():
    """Print classes declared in more than one file. Returns 1 if there is any."""
    repeated = twice()
    print("%d classes are declared in more than one file" % len(repeated))

    for klass in sorted(repeated):
        bases = set(base for unused, base in repeated[klass])
        flag = "   BASES DISAGREE" if len(bases) > 1 else ""
        print("")
        print("%s%s" % (klass, flag))

        for home, base in repeated[klass]:
            print("    %-30s base %s" % (home, base))

    if repeated:
        print("")
        print("Each of these is one object under two declarations. Put the members on one of")
        print("them, delete the other, and include the header that keeps it.")

    return 1 if repeated else 0


def main():
    if "--twice" in sys.argv[1:]:
        return report_twice()

    found = views()
    decoded = decodedClasses()
    print("%d classes with two or more virtual slots, %d of them still views"
          % (len(found), len([v for v in found if v not in decoded])))

    if "--all" in sys.argv[1:]:
        for view in sorted(found):
            home, slots = found[view]
            print("\n%s  (%s, %d slots)" % (view, home, len(slots)))
            for index in sorted(named(slots)):
                print("   %2d  %s/%d -> %s" % (index, slots[index][0], slots[index][1],
                                      slots[index][2]))
        return 0

    names = sorted(found)
    pairs = []

    for i, left in enumerate(names):
        for right in names[i + 1:]:
            if left in decoded and right in decoded:
                continue
            a, b = found[left][1], found[right][1]
            shared = named(a) & named(b)
            if len(shared) < 2:
                continue
            # only named slots can contradict; SlotNN padding says nothing about arity
            if any(a[index][1] != b[index][1] for index in shared):
                continue

            # Return width ranks and does not filter. A pair needs two slots that agree on both
            # arity and width; slots that agree on arity alone are printed as disagreements.
            strong = sorted(index for index in shared if a[index][2] == b[index][2])

            if len(strong) < 2:
                continue

            pairs.append((-(len(strong) * 100 + max(strong)), left, right, strong,
                          sorted(shared - set(strong))))

    pairs.sort()

    print("")
    print("%d pair(s) agree on two or more slots by arity AND return width, strongest first:"
          % len(pairs))

    for weight, left, right, strong, weak in pairs:
        labels = []

        for index in strong:
            name, count, width = found[left][1][index]
            labels.append("%d:%s=%s/%d->%s" % (index, name, found[right][1][index][0], count,
                                               width))

        print("")
        print("    %-26s %-30s %s" % (left, found[left][0], "*" if left in decoded else ""))
        print("    %-26s %-30s %s" % (right, found[right][0], "*" if right in decoded else ""))
        print("        %s" % " ".join(labels))

        for index in weak:
            print("        %d disagrees on width: %s -> %s against %s -> %s"
                  % (index, found[left][1][index][0], found[left][1][index][2],
                     found[right][1][index][0], found[right][1][index][2]))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
