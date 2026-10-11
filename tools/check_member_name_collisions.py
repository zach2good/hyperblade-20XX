# List member names that more than one class declares.
#
# Names like m_nField18 come from an offset, so several classes can share one. A rename keyed on
# the name alone reaches all of them. Run this before such a rename.
#
# A shared name with two types is usually two records. The shape worth a look is a builtin pointer
# beside a class pointer, such as float* in one class and Vec3* in another: the builtin side is
# often the class pointer undecoded. Each such name is open unless SETTLED gives its reason. The
# tool exits 0 either way.
#
#     python tools/check_member_name_collisions.py --list          # every shared name
#     python tools/check_member_name_collisions.py --types         # shared names with two types
#     python tools/check_member_name_collisions.py --name m_nField18   # the classes declaring it

import collections
import os
import re
import sys

MEMBER = re.compile(r"^\s*(?:(?:const|volatile|mutable|static)\s+)*"
                    r"((?:unsigned |signed |struct |class |enum )?[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*\s*\**)"
                    r"\s*(m_[A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*;")
STATEMENTS = ("return", "delete", "new", "case", "else", "throw", "sizeof")

BUILTIN = re.compile(r"^(?:(?:unsigned|signed) )?(?:void|char|short|int|long|float|double)\*+$")

# Names whose builtin pointer was checked and is right.
SETTLED = {
    # TexCacheEntry's is a Texture or a SoftTex, chosen by m_bIsD3D, and the two share no base.
    "m_pTexture": "a variant",
}

# A class head is a name, an optional base list and an optional brace. A member declared
# `class EngineObject* m_pOwner;` is not one.
CLASS = re.compile(r"^\s*(?:class|struct|union)\s+(?:__declspec\s*\([^)]*\)\s*)?([A-Za-z_]\w*)"
                   r"\s*(?::[^:;][^;]*)?\{?\s*$")


def sourceNames(root):
    # Sources live in subdirectories, so this walks. Names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def collect(root):
    owners = collections.defaultdict(set)
    types = collections.defaultdict(set)

    for name in sorted(sourceNames(root)):
        if not name.endswith(".h"):
            continue

        current = None

        for line in open(os.path.join(root, name), encoding="utf-8", errors="replace"):
            if line.lstrip().startswith("//"):
                continue

            m = CLASS.match(line.split("//")[0])
            if m:
                current = m.group(1)
                continue

            m = MEMBER.match(line)

            # `return m_pOwner;` and `return *m_pfl;` in an inline accessor would read as a
            # declaration of type `return`.
            if m and m.group(1).replace("*", " ").split()[0] in STATEMENTS:
                continue

            if m and current:
                owners[m.group(2)].add(current)
                # `class X*` and `X*` are one type.
                spelled = re.sub(r"^(?:struct|class|enum) ", "", m.group(1).strip())
                types[m.group(2)].add(re.sub(r"\s*\*", "*", re.sub(r"\s+", " ", spelled)))

    return owners, types


def main():
    root = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "src")
    owners, types = collect(root)

    if "--name" in sys.argv:
        wanted = sys.argv[sys.argv.index("--name") + 1]
        print("%s: %s" % (wanted, ", ".join(sorted(owners.get(wanted, ["(not declared)"])))))
        return 0

    shared = {n: c for n, c in owners.items() if len(c) > 1}

    if "--list" in sys.argv:
        for name in sorted(shared, key=lambda n: (-len(shared[n]), n)):
            print("%-24s %d classes: %s" % (name, len(shared[name]), ", ".join(sorted(shared[name]))))

    # A shared name is often fine, such as m_pNext in several list classes. The hazard for a
    # name-keyed sweep is a name declared with different types, so those are counted apart.
    clashing = {n: t for n, t in types.items() if len(t) > 1 and len(owners[n]) > 1}

    weakPairs = []

    for name in clashing:
        builtin = [t for t in clashing[name] if BUILTIN.match(t)]
        typed = [t for t in clashing[name] if t.endswith("*") and not BUILTIN.match(t)]

        if builtin and typed:
            weakPairs.append(name)

    openPairs = [n for n in weakPairs if n not in SETTLED]

    if "--types" in sys.argv:
        for name in sorted(clashing, key=lambda n: (-len(clashing[n]), n)):
            mark = ""

            if name in openPairs:
                mark = "  OPEN"
            elif name in weakPairs:
                mark = "  settled: " + SETTLED[name]

            print("%-24s %s%s" % (name, ", ".join(sorted(clashing[name])), mark))

    print("%d member name(s) declared by more than one class, %d of them with more than one type, "
          "%d open: a builtin pointer beside a class pointer, %d settled"
          % (len(shared), len(clashing), len(openPairs), len(weakPairs) - len(openPairs)))

    return 0


if __name__ == "__main__":
    sys.exit(main())
