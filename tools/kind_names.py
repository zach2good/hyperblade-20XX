"""Compare every name the tree gives an object kind with the name the client gives it.

Cmd_DefineObject carries the client's own table of object type names and the id each parses
to, and engineobject.cpp stores that id in EngineObject::m_nKind at 0x20. A define compared
against the kind field names a value the client already has a word for.

A row is a fault when the define's name shares no word with the client's. Rows in CLEARED are
counted and not shown; a cleared row whose define is gone fails.

    python tools/kind_names.py          # disagreements only
    python tools/kind_names.py --all    # every compared define
"""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")
TABLE_FILE = os.path.join(SRC, "engine/gameengine.cpp")
KIND_FILE = "engine/objectkind.h"

# A define whose name disagrees with the client's word and has a reason to, keyed by its path
# under src/.
CLEARED = {
    # NGoal and SGoal both parse to it.
    (KIND_FILE, "KIND_GOAL"),
}


def sourceNames(root):
    # Sources live in subdirectories, so walk rather than list. Names are relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def kind_enum():
    # The ObjectKind names, which the client's table spells its ids with.
    text = open(os.path.join(SRC, KIND_FILE), encoding="utf-8").read()

    return dict((name, int(value, 16))
                for name, value in re.findall(r"^\s*(KIND_\w+)\s*=\s*0x([0-9a-fA-F]+)", text, re.M))


def client_table(kinds):
    text = open(TABLE_FILE, encoding="utf-8").read()
    names = re.search(r"apszTypes\[TYPE_COUNT\]\s*=\s*\{(.*?)\};", text, re.S)
    ids = re.search(r"anTypeIds\[TYPE_COUNT\]\s*=\s*\{(.*?)\};", text, re.S)

    if names is None or ids is None:
        sys.stderr.write("the client's type table is not where this screen looks for it\n")
        sys.exit(2)

    names = re.findall(r'"([^"]+)"', names.group(1))
    unknown = [token for token in re.findall(r"KIND_\w+", ids.group(1)) if token not in kinds]

    if unknown:
        sys.stderr.write("%s does not name %s\n" % (KIND_FILE, ", ".join(unknown)))
        sys.exit(2)

    ids = [kinds[token] if token in kinds else int(token, 16)
           for token in re.findall(r"\b(KIND_\w+|0x[0-9a-fA-F]+)\b", ids.group(1))]

    if len(names) != len(ids):
        sys.stderr.write("the table's two halves are different lengths\n")
        sys.exit(2)

    byid = {}
    for name, value in zip(names, ids):
        byid.setdefault(value, []).append(name)

    # The second numbering. Cmd_DefineObject writes the table index into the definition's 0x04
    # and the id into 0x08. From Multiplier on, index and id differ.
    byindex = {}
    for index, name in enumerate(names):
        byindex.setdefault(index, []).append(name)

    return byid, byindex


def words(name):
    spaced = re.sub(r"([a-z0-9])([A-Z])", r"\1 \2", name)
    return set(w.lower() for w in re.split(r"[^A-Za-z0-9]+", spaced) if w)


def compared_index_names(text):
    """The identifiers the file tests a definition's type index (ObjectDef 0x04) against.

    A file spells the index as a define or reaches it through m_nType.
    """
    sides = []

    if re.search(r"#define\s+\w*DEF_TYPE\s+0x04\b", text):
        sides.append(r"\w*DEF_TYPE\s*\)")

    sides += [r"m_nType\b"]
    sides += [re.escape(h) for h in
              re.findall(r"(\w+)\s*=\s*\*\(int\*\)\s*\([^;]*?DEF_TYPE\s*\)", text)]

    found = set()
    for side in sides:
        found |= set(re.findall(side + r"\s*(?:==|!=)\s*(\w+)", text))
        found |= set(re.findall(r"(\w+)\s*(?:==|!=)\s*" + side, text))

    return found


def compared_names(text):
    """The identifiers the file tests the kind field against.

    A file reads the field through a cast of the offset define, or as pObject->m_nKind. Several
    classes declare an m_nKind, so the second spelling counts only on a pointer declared as an
    EngineObject. Only defines used in a comparison count: other KIND defines are offsets or
    masks.
    """
    holders = set(re.findall(r"(\w+)\s*=\s*\*\(int\*\)\s*\([^;]*?OBJECT_KIND\s*\)", text))
    sides = []

    if re.search(r"#define\s+\w*OBJECT_KIND\s+0x20\b", text):
        sides.append(r"\w*OBJECT_KIND\s*\)")

    sides += [re.escape(h) for h in holders]
    # A third spelling: an entry of the engine's object table read at 0x20 without a define.
    if re.search(r"m_ppObjects[^;]{0,80}\+\s*0x20\s*\)", text):
        sides.append(r"m_ppObjects[^;]{0,80}\+\s*0x20\s*\)")

    sides.append(r"m_ppObjects\s*\[[^]]*\]\s*->\s*m_nKind")

    sides += [re.escape(name) + r"\s*->\s*m_nKind"
              for name in set(re.findall(r"\bEngineObject\s*\*\s*(\w+)\b", text))]

    found = set()
    for side in sides:
        found |= set(re.findall(side + r"\s*(?:==|!=)\s*(\w+)", text))
        found |= set(re.findall(r"(\w+)\s*(?:==|!=)\s*" + side, text))

    return found


def main():
    show_all = "--all" in sys.argv
    kinds = kind_enum()
    byid, byindex = client_table(kinds)
    rows = []
    met = set()

    for dname, value in sorted(kinds.items()):
        clientnames = byid.get(value)

        if clientnames is None:
            rows.append((True, KIND_FILE, dname, hex(value), "(no such kind in the client's table)"))
            continue

        shared = set()
        for cname in clientnames:
            shared |= words(dname) & words(cname)

        clash = not shared

        if (KIND_FILE, dname) in CLEARED:
            if clash:
                met.add((KIND_FILE, dname))
            if not show_all:
                continue

        if clash or show_all:
            rows.append((clash, KIND_FILE, dname, hex(value), "%s (kind)" % "/".join(clientnames)))

    for entry in sorted(sourceNames(SRC)):
        if not entry.endswith(".cpp"):
            continue

        text = open(os.path.join(SRC, entry), encoding="utf-8").read()

        defines = dict(re.findall(r"#define\s+(\w+)\s+(0x[0-9a-fA-F]+|\d+)\b", text))
        defines.update(re.findall(r"^\s*(\w*KIND\w*)\s*=\s*(0x[0-9a-fA-F]+|\d+)\s*,?\s*$",
                                  text, re.M))

        passes = [(compared_names(text), byid, "kind"),
                  (compared_index_names(text), byindex, "index")]

        for names, table, which in passes:
          for dname in sorted(names):
            if dname not in defines:
                continue

            dvalue = defines[dname]
            value = int(dvalue, 0)
            clientnames = table.get(value)

            if clientnames is None:
                rows.append((True, entry, dname, dvalue,
                             "(no such %s in the client's table)" % which))
                continue

            shared = set()
            for cname in clientnames:
                shared |= words(dname) & words(cname)

            clash = not shared

            if (entry, dname) in CLEARED:
                if clash:
                    met.add((entry, dname))
                if not show_all:
                    continue

            if clash or show_all:
                rows.append((clash, entry, dname, dvalue,
                             "%s (%s)" % ("/".join(clientnames), which)))

    rows.sort(key=lambda row: (not row[0], row[1]))

    for row in rows:
        mark = "DISAGREE" if row[0] else "        "
        print("%s %-28s %-32s %-6s %s" % (mark, row[1], row[2], row[3], row[4]))

    # A cleared row shows only under --all and must not fail it.
    bad = sum(1 for row in rows if row[0] and (row[1], row[2]) not in CLEARED)
    stale = sorted(set(CLEARED) - met)

    for entry, dname in stale:
        print("STALE CLEARED %-28s %s: the define it excuses is gone" % (entry, dname))

    print("%d row(s) shown, %d disagree with the client's own word, %d cleared, %d stale"
          % (len(rows), bad, len(CLEARED), len(stale)))

    if bad or stale:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
