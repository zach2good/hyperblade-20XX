# Report a comment in src/ that names a file path the tree does not have.
#
# Paths under src/, docs/, symbols/, tools/ and cmake/ are checked. A path whose basename exists
# elsewhere is reported as MOVED with the new path. Any other is GONE: point the comment at the
# surviving file. A mention that is deliberately about a retired file is registered in
# symbols/source_paths.csv with the reason. tools/check_doc_paths.py does the same for docs/.
#
#   python tools/check_source_paths.py
#   python tools/check_source_paths.py --list    # also confirm when every path resolves
#
# Read only. Exit code 1 when any path is moved or gone.

import collections
import csv
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REGISTER = os.path.join("symbols", "source_paths.csv")

PATH = re.compile(r"\b(?:src|docs|symbols|tools|cmake)/[A-Za-z0-9_/.-]+"
                  r"\.(?:cpp|h|py|md|csv|cmake|txt)\b")

SKIP = ("src/jpeg/",)


def present():
    """Every file under the roots a comment may name, and where each basename is available."""
    have = set()
    byname = collections.defaultdict(list)

    # Walk every root PATH names, or those paths would all read as gone.
    for root in ("src", "docs", "symbols", "tools", "cmake"):
        for directory, unused, names in os.walk(root):
            for name in sorted(names):
                shown = os.path.join(directory, name).replace(os.sep, "/")
                have.add(shown)
                byname[name].append(shown)

    return have, byname


def registered():
    if not os.path.exists(REGISTER):
        return set()

    with open(REGISTER, encoding="utf-8") as handle:
        return {row["path"] for row in csv.DictReader(handle) if row.get("path")}


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    have, byname = present()
    known = registered()

    moved = collections.defaultdict(list)
    gone = collections.defaultdict(list)
    mentions = 0

    for directory, unused, names in os.walk("src"):
        for name in sorted(names):
            if not name.endswith((".cpp", ".h")):
                continue

            shown = os.path.join(directory, name).replace(os.sep, "/")

            if shown.startswith(SKIP):
                continue

            lines = open(shown, encoding="utf-8", errors="replace").read().splitlines()

            for number, line in enumerate(lines, 1):
                at = line.find("//")

                if at < 0:
                    continue

                for hit in PATH.findall(line[at:]):
                    mentions += 1

                    if hit in have or hit in known:
                        continue

                    where = "%s:%d" % (shown, number)

                    if byname.get(os.path.basename(hit)):
                        moved[hit].append(where)
                    else:
                        gone[hit].append(where)

    for hit in sorted(moved):
        print("MOVED %-42s now %-42s %s"
              % (hit, byname[os.path.basename(hit)][0], moved[hit][0]))

    for hit in sorted(gone):
        print("GONE  %-42s %d mention(s), first at %s" % (hit, len(gone[hit]), gone[hit][0]))

    if show and not moved and not gone:
        print("every one of the %d path(s) named in a comment is a file this tree has" % mentions)

    print("%d src path(s) named in comments; %d name one that has moved, %d name one that is "
          "gone and is not registered, %d registered"
          % (mentions, sum(len(v) for v in moved.values()),
             sum(len(v) for v in gone.values()), len(known)))

    return 1 if moved or gone else 0


if __name__ == "__main__":
    sys.exit(main())
