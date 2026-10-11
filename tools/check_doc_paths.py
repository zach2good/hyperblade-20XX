# Check every repository path a document names, and report the ones that no longer exist.
#
# Reads README.md, CONTRIBUTING.md, AGENTS.md, tools/README.md and docs/**/*.md, docs/README.md
# among them.
#
#   MOVED  exactly one file of that basename exists elsewhere. Fix the path.
#   GONE   no file of that basename exists, or several do. A rename reads as GONE. Fix the
#          path, or, when the mention is deliberately about a retired file, register it in
#          symbols/doc_paths.csv with the reason.
#
#   python tools/check_doc_paths.py
#   python tools/check_doc_paths.py --list    # print each path
#
# Exit code 1 when any path is moved, or gone and unregistered.

import collections
import csv
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REGISTER = os.path.join("symbols", "doc_paths.csv")
ROOTS = ("src", "tools", "docs", "symbols", "cmake", "progress")

# A root directory, path characters, and an extension. The extension keeps prose like
# "src/graphics" out.
PATH = re.compile(r"\b((?:" + "|".join(ROOTS) + r")/[A-Za-z0-9_./-]+\.[A-Za-z0-9]+)")


def documents():
    found = [p for p in ("README.md", "CONTRIBUTING.md", "AGENTS.md", "tools/README.md")
             if os.path.exists(p)]

    for directory, unused, names in os.walk("docs"):
        for name in sorted(names):
            if name.endswith(".md"):
                found.append(os.path.join(directory, name).replace(os.sep, "/"))

    return found


def byBasename():
    found = collections.defaultdict(list)

    for root in ROOTS:
        for directory, unused, names in os.walk(root):
            for name in names:
                found[name].append(os.path.join(directory, name).replace(os.sep, "/"))

    return found


def registered():
    if not os.path.exists(REGISTER):
        return set()

    with open(REGISTER, encoding="utf-8") as handle:
        return {row["path"] for row in csv.DictReader(handle) if row.get("path")}


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    known = registered()
    elsewhere = byBasename()
    moved = collections.defaultdict(list)
    gone = collections.defaultdict(list)

    for doc in documents():
        text = open(doc, encoding="utf-8", errors="replace").read()

        for path in PATH.findall(text):
            if os.path.exists(path) or path in known:
                continue

            hits = elsewhere.get(os.path.basename(path), [])

            if len(hits) == 1:
                moved[path].append(doc)
            else:
                gone[path].append(doc)

    if show:
        for path in sorted(moved):
            print("MOVED %-38s -> %-38s %s"
                  % (path, elsewhere[os.path.basename(path)][0],
                     ", ".join(sorted(set(moved[path])))))
        for path in sorted(gone):
            print("GONE  %-38s %s" % (path, ", ".join(sorted(set(gone[path])))))

    print("%d path(s) name a file that has moved, over %d mention(s); "
          "%d name one that is gone and is not registered, %d registered"
          % (len(moved), sum(len(v) for v in moved.values()), len(gone), len(known)))

    return 1 if moved or gone else 0


if __name__ == "__main__":
    sys.exit(main())
