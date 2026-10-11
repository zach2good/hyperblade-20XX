"""Finds files that name a record's offsets differently from the class that covers them.

tools/rival_names.py pairs a define with a field by the define's prefix, which only works when
the file calls the record what the class is called. This takes a blob local whose offsets a
class already covers, as tools/blob_locals.py does, and holds the file's own names for those
offsets against the class's.

A row is a fault when the two names share no word. The register below holds the pairs that
disagree for a reason, and a register entry that no longer exists fails.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import blob_locals
import self_fields

SRC = self_fields.SRC

# A pair that disagrees and has a reason to, keyed by the file's path under src/.
CLEARED = {
}

KINDS = {
    "count": "count", "total": "count", "num": "count", "n": "count",
    "index": "index", "idx": "index", "slot": "index", "entry": "index",
    "flag": "flag", "flags": "flag", "bit": "flag", "bits": "flag", "enabled": "flag",
    "name": "name", "names": "name", "string": "name", "text": "name", "str": "name",
    "ptr": "pointer", "pointer": "pointer", "block": "pointer", "list": "pointer",
    "table": "pointer", "buffer": "pointer",
    "id": "id", "kind": "kind", "type": "kind",
    "frame": "frame", "segment": "segment",
    "x": "axis", "y": "axis", "z": "axis",
    "radius": "extent", "extent": "extent", "size": "extent", "length": "extent",
    "offset": "displacement", "displacement": "displacement", "delta": "displacement",
    "pos": "place", "position": "place", "centre": "place", "center": "place",
}


def sourceNames(root):
    # Walk subdirectories; names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def words(name):
    spaced = re.sub(r"([a-z0-9])([A-Z])", r"\1 \2", name)
    parts = set(w.lower() for w in re.split(r"[^A-Za-z0-9]+", spaced) if w)
    # A Hungarian head says the storage, not the meaning.
    return parts - set(["m", "p", "n", "w", "c", "b", "fl", "a", "ap", "an", "sz", "psz"])


def kinds_of(name):
    return set(KINDS[w] for w in words(name) if w in KINDS)


def main():
    show_all = "--all" in sys.argv
    known = self_fields.records()
    layouts = {}
    rows = []
    met = set()

    for entry in sorted(sourceNames(SRC)):
        if not entry.endswith(".cpp"):
            continue

        path = os.path.join(SRC, entry)
        text = open(path, encoding="utf-8", errors="replace").read()
        macros = dict((key, int(value, 16)) for key, value in self_fields.DEFINE.findall(text))

        for local in sorted(set(re.findall(r"(?<![0-9A-Za-z_])char\*\s+(?:const\s+)?"
                                           r"([0-9A-Za-z_]+)\s*[;=]", text))):
            used = blob_locals.offsets(text, local, macros)

            if not used or blob_locals.pooled(text, local) > 1:
                continue

            says = (blob_locals.claimed(text, local) | blob_locals.owner(text, local)
                    | blob_locals.flattened(text, local))
            best = None

            for klass in says:
                if klass not in layouts:
                    layouts[klass] = self_fields.layout(klass, known)

                covered = sum(1 for offset in used if offset in layouts[klass])

                if best is None or covered > best[1]:
                    best = (klass, covered)

            if best is None or best[1] * 2 < len(used):
                continue

            klass = best[0]

            for macro, value in sorted(macros.items()):
                if value not in used or value not in layouts[klass]:
                    continue

                # The define has to be one this local is read through, not any define in the
                # file that happens to hold the same number.
                if not re.search(r"(?<![0-9A-Za-z_])" + re.escape(local)
                                 + r"\s*\+\s*" + re.escape(macro) + r"(?![0-9A-Za-z_])", text):
                    continue

                field = layouts[klass][value][0]
                shared = kinds_of(macro) & kinds_of(field)
                clash = kinds_of(macro) and kinds_of(field) and not shared

                if (entry, macro) in CLEARED:
                    if clash:
                        met.add((entry, macro))
                    if not show_all:
                        continue

                if clash or show_all:
                    rows.append((clash, entry, local, klass, value, field, macro))

    rows.sort(key=lambda row: (not row[0], row[1], row[4]))

    for clash, entry, local, klass, value, field, macro in rows:
        mark = "DISAGREE" if clash else "        "
        print("%s %-26s %-12s %-22s 0x%03x  %-24s %s"
              % (mark, entry, local, klass, value, field, macro))

    bad = sum(1 for row in rows if row[0] and (row[1], row[6]) not in CLEARED)
    stale = sorted(set(CLEARED) - met)

    for entry, macro in stale:
        print("STALE CLEARED %-26s %s: the pair it excuses is gone" % (entry, macro))

    print("%d pair(s) shown, %d disagree about the kind of value, %d cleared, %d stale"
          % (len(rows), bad, len(CLEARED), len(stale)))

    if bad or stale:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
