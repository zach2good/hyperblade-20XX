# Finds a local that only gives a parameter a second name: `PlanNode* const pPlan = pNode;`
# where pNode is declared PlanNode* and nothing in the body writes it or takes its address.
#
#     python tools/param_aliases.py               open ones, then the noted count
#     python tools/param_aliases.py apply FILE    rewrite every open one in FILE
#
# An alias with a comment directly above it is counted as NOTED and left alone; the comment
# should say why the copy is kept. The rewrite deletes the alias and renames its uses to the
# parameter within the one body. Measure each file on its own: a second name sometimes decides
# register allocation.
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ALIAS = re.compile(r"^(\s+)((?:const\s+)?[\w:]+\s*\**)\s*const\s+(\w+)\s*=\s*(\w+);\s*$")
FUNC = re.compile(r"^[\w:~<>\*&\s]+?\b[\w:~]+\s*\(([^;{}]*)\)\s*(?:const)?\s*$", re.S)


def norm(kind):
    return re.sub(r"\s+", "", kind.replace("class ", "").replace("struct ", ""))


def functions(lines):
    """Yield (params, body_start, body_end) for each top-level function body."""
    i = 0
    n = len(lines)
    while i < n:
        if lines[i] == "{" and i > 0:
            j = i - 1
            head = []
            while j >= 0 and lines[j].strip() and not lines[j].lstrip().startswith("//") \
                    and not lines[j].rstrip().endswith(";") and lines[j] != "}":
                head.insert(0, lines[j])
                j -= 1
            sig = " ".join(head)
            m = FUNC.match(sig.strip())
            k = i + 1
            while k < n and lines[k] != "}":
                k += 1
            if m:
                params = {}
                for part in m.group(1).split(","):
                    part = part.strip()
                    pm = re.match(r"^(.*?)(\w+)\s*$", part)
                    if pm and pm.group(1).strip():
                        params[pm.group(2)] = norm(pm.group(1))
                yield params, i + 1, k
            i = k
        i += 1


def candidates(path):
    text = open(path, encoding="utf-8", newline="").read()
    nl = "\r\n" if "\r\n" in text else "\n"
    lines = text.split(nl)
    found = []
    for params, start, end in functions(lines):
        body = lines[start:end]
        for off, line in enumerate(body):
            m = ALIAS.match(line)
            if not m:
                continue
            kind, alias, source = norm(m.group(2)), m.group(3), m.group(4)
            if source not in params or params[source] != kind:
                continue
            rest = nl.join(body[:off] + body[off + 1:])
            if re.search(r"\b%s\s*(=[^=]|\+\+|--|\+=|-=)" % source, rest) \
                    or re.search(r"(\+\+|--)\s*%s\b" % source, rest) \
                    or re.search(r"&\s*%s\b" % source, rest):
                continue
            noted = start + off > 0 and lines[start + off - 1].strip().startswith("//")
            found.append((start + off, alias, source, noted))
    return lines, nl, found


def main():
    if len(sys.argv) > 1 and sys.argv[1] == "apply":
        for path in sys.argv[2:]:
            lines, nl, found = candidates(path)
            for index, alias, source, noted in reversed(found):
                if noted:
                    continue
                del lines[index]
                end = index
                while lines[end] != "}":
                    end += 1
                for k in range(index, end):
                    lines[k] = re.sub(r"\b%s\b" % alias, source, lines[k])
            open(path, "w", encoding="utf-8", newline="").write(nl.join(lines))
            print(len(found), path)
        return
    open_count = 0
    noted_count = 0
    for path in sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True)):
        _, _, found = candidates(path)
        for index, alias, source, noted in found:
            if noted:
                noted_count += 1
                continue
            open_count += 1
            print("%s:%d  %s = %s" % (os.path.relpath(path, ROOT), index + 1, alias, source))
    print("%d parameter alias(es) open, %d NOTED with a reason above them" % (open_count, noted_count))


main()
