"""Fail when a member stores a `g_vtbl` array that is not its own class's table.

Every table the tree stores is now the compiler's own, so this guards against a manual table
coming back. A store of another class's table compiles and leaves the object holding the wrong
vptr. tools/check_member_owner.py catches a whole body filed on the wrong class; this catches a
body on the right class storing the wrong array.

It also fails on any assignment into the first word of `this` through a cast. A store into
`this` or a known alias of it counts. A store into any other local is a pool seeding a node's
table and is passed over.

    python tools/check_vtbl_stores.py

Exit code 1 when a member stores a table its class is not pinned to.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

# Comment lines may sit between the marker and the class it pins.
PIN = re.compile("// *(?:VTABLE: *HYPERX +(0x[0-9a-fA-F]+)|SLOT TABLE +(0x[0-9a-fA-F]+))"
                 + r"[^" + chr(10) + r"]*" + chr(10)
                 + r"(?://[^" + chr(10) + r"]*" + chr(10) + r")*"
                 + r"(?:class|struct) ([0-9A-Za-z_]+)")

DEFINITION = re.compile(r"^(?:[\w:*&<>\s]+?)\b([0-9A-Za-z_]+)::([~0-9A-Za-z_]+)\s*\([^;{]*\)"
                        + r"\s*(?:const\s*)?" + chr(10) + r"?\{", re.M)

# A store into this. `pObj`, `pThis` and the rest are the body's own `(char*)this` alias.
STORE = re.compile(r"\*\(\s*const\s+void\s*\*(?:\s+const\s*)?\*\s*\)\s*"
                   + r"(?:this|pObj|pThis|pSelf|pManager)\s*=\s*g_vtbl(004[bB][0-9a-fA-F]{4})")


# Any assignment into the first word of `this`, since a returning table need not be spelled
# `g_vtbl004bXXXX`. On a polymorphic class that word is the vptr, which the compiler owns; on a
# plain one it is a field the class should name. An explicit constructor or destructor call is
# not an assignment and does not match.
VPTR = re.compile(r"\*\s*\(\s*[\w:]+\s*\*+\s*\)\s*(?:this|pObj|pThis|pSelf|pManager)\s*=(?!=)")

def sourceNames(root):
    # Sources live in subdirectories, so this walks. Names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def sources(suffixes):
    for name in sorted(sourceNames(SRC)):
        if name.endswith(suffixes):
            text = open(os.path.join(SRC, name), encoding="utf-8", errors="replace").read()

            yield name, text.replace(chr(13) + chr(10), chr(10))


def pins():
    """{class: the table it is pinned to}, over both marker spellings."""
    found = {}

    for unused, text in sources((".h", ".cpp")):
        for first, second, klass in PIN.findall(text):
            found[klass] = int(first or second, 16)

    return found


def bases():
    found = {}

    for unused, text in sources((".h", ".cpp")):
        for derived, base in re.findall(r"^(?:class|struct) ([0-9A-Za-z_]+)\s*:\s*public "
                                        + r"([0-9A-Za-z_]+)", text, re.M):
            found[derived] = base

    return found


def main():
    pinned = pins()
    parent = bases()
    rows = []
    wide = []
    checked = 0

    for name, text in sources((".cpp",)):
        for match in DEFINITION.finditer(text):
            klass = match.group(1)
            end = text.find(chr(10) + "}" + chr(10), match.end())
            body = text[match.end():end if end > 0 else len(text)]

            for unused in VPTR.finditer(body):
                wide.append((name, klass, match.group(2)))
            for word in STORE.findall(body):
                stored = int(word, 16)
                checked += 1

                # Its own table, or one of its bases': a destructor restores the table of the
                # class whose destructor it is, and a base's destructor called from a derived
                # one restores the base's.
                at = klass
                seen = set()
                ok = False

                while at and at not in seen:
                    seen.add(at)

                    if pinned.get(at) == stored:
                        ok = True
                        break

                    at = parent.get(at)

                if not ok:
                    rows.append((name, klass, match.group(2), stored, pinned.get(klass)))

    for name, klass, method in wide:
        print("%s  %s::%s assigns into the first word of this" % (name, klass, method))
    for name, klass, method, stored, own in rows:
        print("%s  %s::%s stores 0x%08x; this class is pinned to %s"
              % (name, klass, method, stored,
                 "0x%08x" % own if own else "no table at all"))

    print("%d store(s) into this, %d not the class's own table or a base's, "
          "%d assignment(s) into the first word of this"
          % (checked, len(rows), len(wide)))

    if wide:
        print("The first word of an object is the vptr on a polymorphic class and a field on")
        print("every other one. Neither is written through a cast of this: name the field, or")
        print("let the constructor the compiler generates store the table.")
        return 1
    if rows:
        print("A class writing another class's table leaves the object holding the wrong vptr.")
        print("Either the body is on the wrong class, which check_member_owner.py would also")
        print("say, or the array is the wrong one.")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
