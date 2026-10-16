"""Fail when a plan function's name is not the developers' for its id, from ENUMIDS.H or a cache.

The plan machine dispatches through two function pointer tables, and a body's slot is its id in
DATA/ENUMIDS.H, which ships with the game. The opcode table runs two slots ahead of
DecisionFunctionIDs: slots zero and one have no enum entry. Bodies registered in both tables
agree by name only at this shift.

A row is a fault when the name is not the developers' name. CLEARED
holds rows that disagree for a reason; a cleared row whose registration is gone also fails.

The scope names the table: PlanAction:: for the action table, PlanCond:: for the opcode table.

    python tools/plan_names.py          # the disagreements
    python tools/plan_names.py --all    # every row
    python tools/plan_names.py --emit   # rewrite the cache from the header

Reads ENUMIDS.H from build/out/Data or HB_ENUMIDS. tools/plan_names.json holds the developer
name at each id the tree registers a body at, and nothing else. With the header, the tool also
fails when that cache is stale. Without it, the tool checks every row against the cache, and an
id the cache lacks fails as UNCACHED. Without the cache too, it exits 2.
"""

import json
import os
import re
import sys

import plan_tables

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")
ENUMIDS = os.environ.get("HB_ENUMIDS", os.path.join(ROOT, "build", "out", "Data", "ENUMIDS.H"))
CACHE = os.path.join(ROOT, "tools", "plan_names.json")

TABLES = [
    ("g_pfnPlanActionHandlers", "ActionFunctionIDs", 0),
    ("g_pfnPlanOpcodeHandlers", "DecisionFunctionIDs", -2),
]

# A body whose name disagrees with its id's and has a reason to.
CLEARED = {
    ("g_pfnPlanOpcodeHandlers", 10, "PlanCond::SetResultDirect"):
        "one body registered at two ids, so it can carry neither name.",
    ("g_pfnPlanOpcodeHandlers", 36, "PlanCond::SetResultDirect"): "as opcode 10.",
    ("g_pfnPlanOpcodeHandlers", 77, "PlanCond::DecideNever"):
        "one body registered at three ids.",
    ("g_pfnPlanOpcodeHandlers", 78, "PlanCond::DecideNever"): "as opcode 77.",
    ("g_pfnPlanOpcodeHandlers", 79, "PlanCond::DecideNever"): "as opcode 77.",
    ("g_pfnPlanOpcodeHandlers", 105, "PlanCond::GetTeamParity"):
        "one body registered at two ids, so it can carry neither name. Its other slot is 93, "
        "where id_DCF_HomeOrAwayTeam happens to share a word with it.",
    ("g_pfnPlanOpcodeHandlers", 0, "PlanCond::SetInteractorName"):
        "slot nought, which is one of the two the enum has no entry for.",
    ("g_pfnPlanOpcodeHandlers", 1, "PlanCond::SetActiveState"): "as opcode 0.",
    ("g_pfnPlanOpcodeHandlers", 93, "PlanCond::GetTeamParity"): "as opcode 105.",
    ("g_pfnPlanActionHandlers", 5, "PlanAction::SendMsg"):
        "windows.h defines SendMessage as a macro, which would rename the member.",
    ("g_pfnPlanActionHandlers", 7, "AnimEvent::PlaySnd"):
        "windows.h defines PlaySound as a macro, which would rename the member.",
    ("g_pfnPlanActionHandlers", 55, "AnimEvent::PlaySoundByDistance"): "as action 7.",
    ("g_pfnPlanActionHandlers", 118, "PlanAction::SubstitutePlayer"):
        "one body registered in both tables, as SubstituteDromer and OkToSubstituteDromer.",
    ("g_pfnPlanOpcodeHandlers", 112, "PlanAction::SubstitutePlayer"): "as action 118.",
}


def enum(name):
    text = open(ENUMIDS, encoding="latin-1").read()
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    match = re.search(r"enum\s+" + name + r"\s*\{(.*?)\}", text, re.S)

    if match is None:
        sys.stderr.write("no %s in %s\n" % (name, ENUMIDS))
        sys.exit(2)

    return re.findall(r"(id_\w+)", re.sub(r"//[^\n]*", "", match.group(1)))


# Words common to many of these names, which say nothing about which one this is. A match on a
# shared verb such as "play" or "set" is not a match. Check the whole table after adding one.
VERBS = set(["set", "get", "do", "is", "on", "to", "the", "a", "of", "and", "or", "my", "it",
             "id", "acf", "dcf", "plan", "cond", "action", "op", "update", "check", "handle",
             "play", "send", "start", "end", "note"])


def words(name, strip=True):
    spaced = re.sub(r"([a-z0-9])([A-Z])", r"\1 \2", name)
    spaced = re.sub(r"([A-Za-z])([0-9])", r"\1 \2", spaced)
    found = set(w.lower() for w in re.split(r"[^A-Za-z0-9]+", spaced) if w)

    if not strip:
        return found

    # A name made only of stop words keeps them, or MyID would share nothing with id_DCF_MyID.
    if found - VERBS:
        return found - VERBS

    return found



def compared(registered):
    """Each (table, enum, slot, index, body) whose name the check compares."""
    found = []

    for table, enumname, shift in TABLES:
        slots = [(n, body) for t, n, body in registered if t == table]

        if not slots:
            sys.stderr.write("no registrations for %s\n" % table)
            sys.exit(2)

        for slot, body in sorted(slots):
            if body == "Stub::Assert" or "::" not in body:
                continue

            found.append((table, enumname, int(slot), int(slot) + shift, body))

    return found


def emitted(rows):
    """Each enum's length, and the developer name at each index a registration reaches."""
    lists = dict((enumname, enum(enumname)) for table, enumname, shift in TABLES)
    cache = dict((name, {"count": len(names), "names": {}}) for name, names in lists.items())

    for table, enumname, slot, index, body in rows:
        names = lists[enumname]

        if 0 <= index < len(names):
            cache[enumname]["names"][str(index)] = names[index]

    return cache


def main():
    show_all = "--all" in sys.argv
    live = os.path.exists(ENUMIDS)
    cached = None

    if os.path.exists(CACHE):
        cached = json.load(open(CACHE, encoding="utf-8"))["enums"]

    if not live and "--emit" in sys.argv:
        print("--emit needs %s, which is not there" % ENUMIDS)
        return 2

    if not live and cached is None:
        print("no %s and no %s, so nothing can be checked" % (ENUMIDS, CACHE))
        return 2

    compare = compared(plan_tables.registrations())
    known = cached
    stale_cache = False

    if live:
        known = emitted(compare)

        if "--emit" in sys.argv:
            with open(CACHE, "w", encoding="utf-8") as handle:
                json.dump({"comment": "The developer name ENUMIDS.H gives each plan handler id "
                                      "the tree registers a body at, and each enum's length, "
                                      "from tools/plan_names.py --emit.",
                           "enums": known}, handle, indent=1, sort_keys=True)
                handle.write("\n")

            print("%s written for %d enum(s)" % (CACHE, len(known)))
            return 0

        stale_cache = known != cached

    rows = []
    met = set()

    for table, enumname, slot, index, body in compare:
        table_names = known[enumname]["names"]

        if not 0 <= index < known[enumname]["count"]:
            dev = "(no id at that index)"
            clash = True
        elif str(index) not in table_names:
            dev = "(UNCACHED: check it with ENUMIDS.H, then --emit)"
            clash = True
        else:
            dev = table_names[str(index)]
            clash = body.split("::")[-1].lower() != dev.split("_", 2)[-1].lower()

        key = (table, slot, body)

        if key in CLEARED:
            if clash:
                met.add(key)
            if not show_all:
                continue

        if clash or show_all:
            rows.append((clash, table, slot, body, dev))

    rows.sort(key=lambda row: (not row[0], row[1], row[2]))

    for clash, table, slot, body, dev in rows:
        mark = "DISAGREE" if clash else "        "
        print("%s %-24s %4d  %-46s %s" % (mark, table[5:], slot, body, dev))

    bad = sum(1 for row in rows
              if row[0] and (row[1], row[2], row[3]) not in CLEARED)
    stale = sorted(set(CLEARED) - met)

    for table, slot, body in stale:
        print("STALE CLEARED %-24s %4d %s: that registration is gone" % (table[5:], slot, body))

    if stale_cache:
        print("STALE CACHE %s no longer matches %s: run --emit" % (CACHE, ENUMIDS))

    source = "the header"

    if not live:
        source = "the cache"

    print("%d row(s) shown, %d disagree with the developers' own name, %d cleared, %d stale, "
          "checked against %s" % (len(rows), bad, len(CLEARED), len(stale), source))

    if bad or stale or stale_cache:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
