"""Fail when an id the tree names disagrees with the developers' name, from ENUMIDS.H or a cache.

DATA/ENUMIDS.H ships with the game. It is the developers' own header of ids for models, object
types, messages, collision objects, ratings, stats, behaviours and plan functions. A define in
src/ whose value lands on an entry sharing no word with it is reported as DISAGREE. A developer
name that only numbers a slot (`Fighting0`, `Pass2`) is shown as SLOT No. with --all and is not
counted.

    python tools/enum_names.py          # disagreements only
    python tools/enum_names.py --all    # every checked define
    python tools/enum_names.py --emit   # rewrite the cache from the header

The header is game data and is not in this repository. The tool reads it from build/out/Data,
or from HB_ENUMIDS. tools/enum_names.json holds the developer name of each value the tree's
defines reach, and nothing else. With the header, the tool also fails when that cache is stale.
Without it, the tool checks every define against the cache, and a value the cache lacks fails as
UNCACHED. Without the cache too, it exits 2. Without the header it cannot tell a slot name from
a disagreement, so a SLOT No. row fails there. Exit code 1 on a disagreement, a stale cache or a
stale CLEARED entry.
"""

import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")
ENUMIDS = os.environ.get("HB_ENUMIDS", os.path.join(ROOT, "build", "out", "Data", "ENUMIDS.H"))
CACHE = os.path.join(ROOT, "tools", "enum_names.json")

# Which define prefixes in src/ carry which of the header's enums. A prefix is matched at the
# start of the define's name, longest first.
FAMILIES = [
    ("COB_", "CollisionObjectID"),
    ("GOALIE_RATING_", "GoalieRatingsID"),
    ("GRT_", "GoalieRatingsID"),
    ("RATING_", "RatingMeasureID"),
    ("MATCH_STAT_", "StatID"),
    ("MODEL_ID_", "ID_MOD"),
    ("MESSAGE_", "ID_MSG"),
    ("MSG_", "ID_MSG"),
    ("ATTACK_", "ID_ATK"),
    ("BEH_", "Behavior_IDs"),
    ("ACF_", "ActionFunctionIDs"),
    ("DCF_", "DecisionFunctionIDs"),
    ("MON_", "KeyMonitorModes"),
    ("POS_", "ID_POS"),
]

# A family whose ids follow the header only on one channel. The plan message id is the first
# argument to PostSceneEvent; other MSG_ defines are network, Win32 or string-table numbers.
CHANNEL = {
    "ID_MSG": r"PostSceneEvent\s*\(\s*(?:\(int\)\s*)?(\w+)",
}

# A bound or a displacement rather than an id. These end a define's name and take it out.
NOT_IDS = ("FIRST", "LAST", "COUNT", "LIMIT", "MAX", "MIN", "BASE", "STRIDE", "SIZE",
           "OFFSET", "MASK", "BYTES", "WORD", "ROW", "STEP", "CEILING", "FLOOR", "NONE",
           "DEFAULT", "CURRENT", "RATING", "ENTRY", "SLOT", "INDEX", "SHIFT", "BITS",
           "VALUE", "LENGTH", "DURATION", "PARAM", "SCALE", "GATE", "TICKS", "MS")

# A define whose name disagrees and has a reason to.
CLEARED = {}


def sourceNames(root):
    # Sources live in subdirectories, so walk rather than list. Names are relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def enums():
    if not os.path.exists(ENUMIDS):
        return None

    text = open(ENUMIDS, encoding="latin-1").read()
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    found = {}

    for match in re.finditer(r"enum\s+(\w+)\s*\{(.*?)\}", text, re.S):
        body = re.sub(r"//[^\n]*", "", match.group(2))
        names = re.findall(r"(id_\w+)", body)

        if names:
            found[match.group(1)] = names

    return found


def words(name):
    spaced = re.sub(r"([a-z0-9])([A-Z])", r"\1 \2", name)
    spaced = re.sub(r"([A-Za-z])([0-9])", r"\1 \2", spaced)
    return set(w.lower() for w in re.split(r"[^A-Za-z0-9]+", spaced) if w)


def shares(left, right):
    """Whether two sets of words have one in common, allowing an ending.

    Land and Landed are the same word, and so are Arrive and Arrived. A four letter stem is
    enough to stop Pass matching Passive.
    """
    for one in left:
        for other in right:
            if one == other:
                return True

            if len(one) >= 4 and other.startswith(one):
                return True

            if len(other) >= 4 and one.startswith(other):
                return True

    return False



def family_of(dname, text):
    """The prefix and the enum it belongs to, or (None, None) when nothing checks it.

    A STAT_ define belongs to one of two lists, and the accessor settles which. Ratings at 0xd20
    are indexed by RatingMeasureID through GetScalar and GetWeightedStat; match counters at
    0xf24 by StatID through AccumulateStat, StartTimer and StopTimer. One file can reach both.
    """
    code = re.sub(r"//[^\n]*", "", text)

    for prefix, enumname in FAMILIES:
        if not dname.startswith(prefix):
            continue

        if enumname != "RatingMeasureID":
            return prefix, enumname

        name = re.escape(dname)

        if re.search(r"(?:GetScalar|GetWeightedStat)\s*\(\s*" + name + r"\s*\)", code):
            return prefix, "RatingMeasureID"

        if re.search(r"(?:AccumulateStat|StartTimer|StopTimer)\s*\(\s*" + name, code):
            return prefix, "StatID"

        # No accessor names it, so use the block the file reaches.
        if re.search(r"0xf24", code) and not re.search(r"0xd20", code):
            return prefix, "StatID"

        return prefix, "RatingMeasureID"

    return None, None



def checked():
    """Each (file, define, value, prefix, enum) the tree's defines put in a family."""
    found = []
    texts = {}

    for entry in sorted(sourceNames(SRC)):
        if entry.endswith((".cpp", ".h")):
            texts[entry] = open(os.path.join(SRC, entry), encoding="utf-8").read()

    # A shared header defines ids that other files pass, so a channel is searched tree-wide.
    channelled = {}

    for enumname, pattern in CHANNEL.items():
        channelled[enumname] = set()

        for text in texts.values():
            channelled[enumname].update(re.findall(pattern, re.sub(r"//[^\n]*", "", text)))

    for entry, text in texts.items():
        for dname, dvalue in re.findall(r"#define\s+(\w+)\s+(0x[0-9a-fA-F]+|\d+)\s*$",
                                        text, re.M):
            prefix, enumname = family_of(dname, text)

            if enumname is None:
                continue

            if enumname in CHANNEL:
                if dname not in channelled[enumname]:
                    continue

            tail = dname.split("_")[-1]

            if tail in NOT_IDS:
                continue

            found.append((entry, dname, dvalue, prefix, enumname))

    return found


def emitted(tables, defines):
    """Each enum's length, and the developer name of each value a define reaches."""
    cache = {}

    for entry, dname, dvalue, prefix, enumname in defines:
        if enumname not in tables:
            continue

        names = tables[enumname]
        table = cache.setdefault(enumname, {"count": len(names), "names": {}})
        value = int(dvalue, 0)

        if value < len(names):
            table["names"][str(value)] = names[value]

    return cache


def main():
    show_all = "--all" in sys.argv
    tables = enums()
    defines = checked()
    cached = None

    if os.path.exists(CACHE):
        cached = json.load(open(CACHE, encoding="utf-8"))["enums"]

    if tables is None and "--emit" in sys.argv:
        print("--emit needs %s, which is not there" % ENUMIDS)
        return 2

    if tables is None and cached is None:
        print("no %s and no %s, so nothing can be checked" % (ENUMIDS, CACHE))
        return 2

    known = cached
    stale_cache = False

    if tables is not None:
        known = emitted(tables, defines)

        if "--emit" in sys.argv:
            with open(CACHE, "w", encoding="utf-8") as handle:
                json.dump({"comment": "The developer name ENUMIDS.H gives each value the tree's "
                                      "defines reach, and each enum's length, from "
                                      "tools/enum_names.py --emit.",
                           "enums": known}, handle, indent=1, sort_keys=True)
                handle.write("\n")

            print("%s written for %d enum(s)" % (CACHE, len(known)))
            return 0

        stale_cache = known != cached

    rows = []
    met = set()

    for entry, dname, dvalue, prefix, enumname in defines:
        if tables is not None and enumname not in tables:
            continue

        value = int(dvalue, 0)
        table = known.get(enumname)

        if table is not None and value >= table["count"]:
            rows.append((True, entry, dname, dvalue, "(past the end of %s)" % enumname))
            continue

        if table is None or str(value) not in table["names"]:
            rows.append((True, entry, dname, dvalue,
                         "(UNCACHED: check it with ENUMIDS.H, then --emit)"))
            continue

        devname = table["names"][str(value)]

        # A developer name ending in a digit numbers a slot rather than saying what it is
        # for, so a different reader's name for it is not a contradiction.
        positional = re.search(r"[0-9][a-z]?$", devname) is not None

        # Unless the reader's name matches another entry in the same enum. Without the header
        # there is no other entry to look at, so the slot name excuses nothing.
        mine = words(dname) - words(prefix)
        elsewhere = tables is None

        if tables is not None:
            for other, othername in enumerate(tables[enumname]):
                if other != value and shares(mine, words(othername) - words("id RAT GRT STA MOD MSG")):
                    elsewhere = True
                    break

        if elsewhere:
            positional = False

        # The id_XXX_ prefix is the family, not part of the name.
        clash = not shares(words(dname) - words(prefix),
                           words(devname) - words(devname.split("_")[1]))

        if (entry, dname) in CLEARED:
            if clash:
                met.add((entry, dname))
            if not show_all:
                continue

        if positional and clash:
            if show_all:
                rows.append((None, entry, dname, dvalue, devname))
            continue

        if clash or show_all:
            rows.append((clash, entry, dname, dvalue, devname))

    rows.sort(key=lambda row: (row[0] is not True, row[0] is not None, row[1], row[2]))

    for row in rows:
        mark = "        "
        if row[0] is True:
            mark = "DISAGREE"
        elif row[0] is None:
            mark = "SLOT No."
        print("%s %-26s %-34s %-6s %s" % (mark, row[1], row[2], row[3], row[4]))

    bad = sum(1 for row in rows if row[0] is True and (row[1], row[2]) not in CLEARED)
    stale = sorted(set(CLEARED) - met)

    for entry, dname in stale:
        print("STALE CLEARED %-26s %s: the define it excuses is gone" % (entry, dname))

    if stale_cache:
        print("STALE CACHE %s no longer matches %s: run --emit" % (CACHE, ENUMIDS))

    source = "the header"

    if tables is None:
        source = "the cache"

    print("%d row(s) shown, %d disagree with the developers' own name, %d cleared, %d stale, "
          "checked against %s" % (len(rows), bad, len(CLEARED), len(stale), source))

    if bad or stale or stale_cache:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
