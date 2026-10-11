"""Find offsets that a header names one thing and a .cpp's #define names another.

A second name for the same offset is a strong sign that one of the names is wrong.

    python tools/rival_names.py               # pairs whose names disagree
    python tools/rival_names.py --all         # every pair, agreeing ones included

A define is paired with a class field when the .cpp names the class, the define's prefix names
the class, and the define is used as `+ NAME`, not as a mask. A pair is reported as DISAGREE
when the two names claim different kinds of value (a scale against an angle, a name against an
id). Most pairs are the same idea said twice and are not faults.

Known limits: a define that is never used is not seen (tools/check_dead_defines.py covers
those). The screen cannot see which pointer a define is added to, so a prefix that fits two
structures in one file gives a false positive. CLEARED holds those, and an entry that stops
matching is reported as stale. Exits 1 on any disagreement or stale entry.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

FIELD = re.compile(r"^\s{2,}(?:const\s+)?[A-Za-z_][\w:*&\s]*?([A-Za-z_]\w*)\s*(?:\[[^\]]*\])?\s*;"
                   r"\s*//\s*(0x[0-9a-fA-F]+)")
DEFINE = re.compile(r"^#define\s+([A-Z][A-Z0-9_]*)\s+(0x[0-9a-fA-F]+)\s*$", re.M)
CLASSDECL = re.compile(r"^\s*(?:class|struct)\s+([A-Za-z_]\w*)\s*(?::|$|\{)", re.M)

# Words that name a kind of value. A pair that disagrees on these reads different sorts of value
# out of one field.
KINDS = ("scale", "fov", "angle", "azimuth", "heading", "mass", "friction", "damping",
         "elastic", "speed", "accel", "name", "id", "count", "flag", "index", "slot",
         "offset", "size", "time", "weight", "colour", "palette", "kind", "type")

# Spellings that must not read as a disagreement.
SAME = {"color": "colour"}


# Pairs that have been checked and are not a misname, with the reason. A row that stops being
# reported is stale and should be deleted. The key is the file's path under src/.
CLEARED = {
}


def sourceNames(root):
    # Every file under root, recursively, as a path relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def words(name):
    spaced = re.sub(r"([a-z0-9])([A-Z])", r"\1 \2", name)
    return set(w.lower() for w in re.split(r"[^A-Za-z0-9]+", spaced) if w)


def kinds_of(name):
    found = set()
    for w in words(name):
        w = SAME.get(w, w)
        for k in KINDS:
            if k in w or (k in SAME.values() and any(a in w for a, b in SAME.items() if b == k)):
                found.add(k)
    return found


def header_fields():
    """{class: {offset: field}} for every header."""
    out = {}
    for name in sorted(sourceNames(SRC)):
        if not name.endswith(".h"):
            continue
        current = None
        for line in open(os.path.join(SRC, name), encoding="utf-8", errors="replace"):
            decl = CLASSDECL.match(line)
            if decl is not None:
                current = decl.group(1)
                out.setdefault(current, {})
                continue
            hit = FIELD.match(line)
            if hit is not None and current is not None:
                out[current].setdefault(int(hit.group(2), 16), hit.group(1))
    return out


def main():
    show_all = "--all" in sys.argv[1:]
    fields = header_fields()
    pairs = []
    met = set()

    for name in sorted(sourceNames(SRC)):
        if not name.endswith(".cpp"):
            continue
        text = open(os.path.join(SRC, name), encoding="utf-8", errors="replace").read()
        defines = DEFINE.findall(text)
        if not defines:
            continue

        # Keep only defines used as an offset (`+ NAME`), not as a mask.
        defines = [(n, h) for n, h in defines
                   if re.search(r"[+]\s*%s(?![0-9A-Za-z_])" % re.escape(n), text)
                   and not re.search(r"[&|]\s*%s(?![0-9A-Za-z_])" % re.escape(n), text)]
        for klass, byoffset in fields.items():
            if not byoffset or not re.search(r"(?<![0-9A-Za-z_])%s(?![0-9A-Za-z_])"
                                             % re.escape(klass), text):
                continue

            classwords = words(klass)

            for dname, dhex in defines:
                offset = int(dhex, 16)
                field = byoffset.get(offset)
                if field is None:
                    continue

                # The define's prefix must name the class: CAMERA_FOV fits CameraObject,
                # TEAM_NAME does not fit EngineObject.
                prefix = dname.split("_")[0].lower()

                # An exact match on a class word counts at any length (DEF_ against ObjectDef).
                # A substring match needs a word over three letters, so "in" does not pair
                # with "interactor".
                exact = prefix in classwords
                loose = any(prefix in w or w in prefix for w in classwords if len(w) > 3)

                if not exact and not loose:
                    continue
                shared = kinds_of(field) & kinds_of(dname)
                clash = kinds_of(field) and kinds_of(dname) and not shared
                if (name, dname) in CLEARED:
                    if clash:
                        met.add((name, dname))
                    if not show_all:
                        continue

                if clash or show_all:
                    pairs.append((clash, name, klass, dhex, field, dname))

    pairs.sort(key=lambda row: (not row[0], row[1]))
    for clash, cpp, klass, dhex, field, dname in pairs:
        mark = "DISAGREE" if clash else "        "
        print("%s %-26s %-22s %-7s %-24s %s" % (mark, cpp, klass, dhex, field, dname))

    bad = sum(1 for p in pairs if p[0])
    stale = sorted(set(CLEARED) - met)

    for cpp, dname in stale:
        print("STALE CLEARED %-26s %s: the pair it excuses is gone, so the entry is dead" %
              (cpp, dname))

    print("%d pair(s) shown, %d disagree about the kind of value, %d cleared and not shown, "
          "%d cleared entry(s) stale" % (len(pairs), bad, len(CLEARED), len(stale)))

    if bad or stale:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
