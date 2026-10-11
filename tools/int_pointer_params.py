# Lists parameters declared `int` or `void*` whose own body casts them back to a pointer.
#
# The caller casts an address down to an int and the callee casts it back. Removing both casts
# costs no bytes, since the argument is a dword either way. The evidence is usually the body's
# first line:
#
#     void PlanAction::Bind(int nNode)
#     {
#         PlanNode* const pPlan = (PlanNode*)nNode;   <- this
#
# Each hit reports the cast's target type and how many distinct types the body casts to. One
# type is a retype. Several means the parameter is a flag at some sites and an address at others,
# and cannot be typed.
#
# A hit is fixed, and not open, when one of these holds:
#
#   callback   The function's address is taken, so a callback or table type fixes its signature:
#              an API enumeration callback, a qsort comparator, a loader handed to a wrapper.
#   slot       The body assigns the parameter, reusing its stack slot as a local of the declared
#              type. A pointer type cannot hold what the body stores there.
#   family     The function is a virtual slot and the family's bodies cast the parameter to more
#              than one type, so each override receives a different object.
#   integer    A caller passes an int parameter an argument with no cast. A pointer needs a cast to
#              become an int, so that caller passes an integer and the slot carries both.
#
# A function pointer table filled from data, such as the plan opcode table, is not visible here.
# Grep the name for a table before touching an open hit.
#
# Read only.

import collections
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# A definition: a qualified or plain name, its parameter list, and an opening brace with only a
# constructor's initialiser list allowed in between.
DEFN = re.compile(
    r"^(?:[\w:~*&<>\[\]]+[ \t]+)*?([\w:~]+)[ \t]*\(([^;{}]*?)\)[ \t]*(?:const[ \t]*)?"
    r"(?::[^;{}]*?)?\n?\{",
    re.M)
# One parameter, keeping the name: `int nNode`, `void* pDef`, `const void *pThing`.
INT_PARAM = re.compile(r"^\s*(?:const\s+)?int\s+(\w+)\s*$")
VOID_PARAM = re.compile(r"^\s*(?:const\s+)?void\s*\*\s*(\w+)\s*$")
# The cast back. `(class Foo*)nNode`, `(Foo *)pDef`, `(const Foo**)p`.
CAST = r"\(\s*(?:const\s+)?(?:class\s+|struct\s+)?(\w+)\s*\*+\s*\)\s*%s\b"
# An assignment to the parameter itself, not to a member of the same name or a comparison.
ASSIGN = r"(?<![\w.>])%s\s*=(?!=)"
# A virtual declaration's name.
VIRTUAL = re.compile(r"\bvirtual\b[^;{}()]*?\b(\w+)\s*\(")
# An argument that opens with a cast: `(int)p`, `(unsigned int)p`, `(const Vec3*)p`.
LEADING_CAST = re.compile(r"^\(\s*(?:const\s+)?(?:unsigned\s+)?\w+\s*\**\s*\)")
# An argument that is a parameter declaration, which marks a declaration and not a call.
DECLARATION = re.compile(r"^\s*(?:const\s+)?[\w:]+[\s*&]+\w+\s*$")

SKIP = ("void", "char", "int", "short", "long", "unsigned", "float", "double", "FILE")

REASONS = (
    ("callback", "fixed by a callback or table signature"),
    ("slot", "fixed by a stack slot the body reuses"),
    ("family", "fixed by a virtual slot its family types differently"),
    ("integer", "fixed by callers that pass an integer"),
)


def sources(root, suffixes):
    found = []

    for directory, unused, names in os.walk(root):
        for name in sorted(names):
            if name.endswith(suffixes):
                found.append(os.path.join(directory, name))

    return found


def body(text, at):
    """The braced body starting at the opening brace, by depth. Bounded by the file's length."""
    depth = 0

    for end in range(at, len(text)):
        if text[end] == "{":
            depth += 1
        elif text[end] == "}":
            depth -= 1

            if depth == 0:
                return text[at:end]

    return text[at:]


def arguments(text, at):
    """The top-level arguments of the call whose open parenthesis is at `at`, and the index past
    its close. Bounded by the file's length."""
    depth = 0
    start = at + 1
    found = []

    for end in range(at, len(text)):
        if text[end] == "(":
            depth += 1
        elif text[end] == ")":
            depth -= 1

            if depth == 0:
                last = text[start:end].strip()

                if last or found:
                    found.append(last)

                return found, end + 1
        elif text[end] == "," and depth == 1:
            found.append(text[start:end].strip())
            start = end + 1

    return found, len(text)


def marker(raw, line):
    """The address off the nearest marker above the definition, which names the row to measure."""
    above = "\n".join(raw.split("\n")[:line - 1])
    found = re.findall(r"//\s*(?:FUNCTION|SYNTHETIC|LIBRARY):\s*\w+\s+(0x[0-9a-fA-F]+)", above)

    if not found:
        return "-"

    return found[-1]


def scan(path, shown):
    raw = open(path, encoding="utf-8", errors="replace").read()
    text = code(raw)
    found = []

    for hit in DEFN.finditer(text):
        name = hit.group(1)
        params = hit.group(2).split(",")
        inner = body(text, text.index("{", hit.end() - 1))

        for index, one in enumerate(params):
            asInt = INT_PARAM.match(one)
            asVoid = VOID_PARAM.match(one)

            if not asInt and not asVoid:
                continue

            param = (asInt or asVoid).group(1)
            casts = collections.Counter()

            for kind in re.findall(CAST % re.escape(param), inner):
                if kind not in SKIP:
                    casts[kind] += 1

            if not casts:
                continue

            at = text.count("\n", 0, hit.start()) + 1
            reused = re.search(ASSIGN % re.escape(param), inner) is not None

            found.append({
                "shown": shown, "at": at, "addr": marker(raw, at), "name": name,
                "param": param, "kind": "int" if asInt else "void*", "casts": casts,
                "index": index, "count": len(params), "reused": reused,
            })

    return found


def taken(corpus, name):
    """Whether the function's address is taken anywhere: named with no call after it."""
    if "::" not in name:
        return re.search(r"&\s*%s\b" % re.escape(name), corpus) is not None

    return re.search(r"(?<![\w:])%s\s*[,);]" % re.escape(name), corpus) is not None


def integer_callers(corpus, hit):
    """The calls that hand the parameter an argument with no cast, which is an integer."""
    last = hit["name"].split("::")[-1]
    pattern = r"(?:->|\.)\s*%s\s*\(" % re.escape(last)

    if "::" in hit["name"]:
        pattern = r"(?:(?:->|\.)\s*%s|(?<![\w:])%s)\s*\(" % (re.escape(last),
                                                            re.escape(hit["name"]))

    found = 0

    for call in re.finditer(pattern, corpus):
        args, after = arguments(corpus, call.end() - 1)
        rest = corpus[after:after + 64].lstrip()

        if rest.startswith("{") or rest.startswith("const"):
            continue

        if len(args) != hit["count"]:
            continue

        if any(DECLARATION.match(arg) for arg in args):
            continue

        arg = args[hit["index"]]

        if arg == "0" or LEADING_CAST.match(arg):
            continue

        found += 1

    return found


def classify(hits, corpus, virtuals):
    families = collections.defaultdict(set)

    for hit in hits:
        last = hit["name"].split("::")[-1]

        if last in virtuals:
            families[last].update(hit["casts"])

    for hit in hits:
        last = hit["name"].split("::")[-1]

        if taken(corpus, hit["name"]):
            hit["reason"] = "callback"
        elif hit["reused"]:
            hit["reason"] = "slot"
        elif last in virtuals and len(families[last]) > 1:
            hit["reason"] = "family"
        elif hit["kind"] == "int" and integer_callers(corpus, hit) > 0:
            hit["reason"] = "integer"
        else:
            hit["reason"] = None


def main():
    os.chdir(ROOT)
    quiet = "--count" in sys.argv

    hits = []
    texts = []

    for path in sources("src", (".cpp", ".h")):
        raw = open(path, encoding="utf-8", errors="replace").read()
        texts.append(code(raw))

        if path.endswith(".cpp"):
            shown = os.path.relpath(path, "src").replace(os.sep, "/")
            hits.extend(scan(path, shown))

    corpus = "\n".join(texts)
    virtuals = set(VIRTUAL.findall(corpus))

    classify(hits, corpus, virtuals)

    open_hits = [h for h in hits if h["reason"] is None]
    one = [h for h in open_hits if len(h["casts"]) == 1]
    several = [h for h in open_hits if len(h["casts"]) > 1]

    if not quiet:
        labels = [(None, "")] + list(REASONS)

        for reason, unused in labels:
            for hit in [h for h in hits if h["reason"] == reason]:
                verdict = reason or ("ONE" if len(hit["casts"]) == 1 else "SEVERAL")
                print("%-8s %s:%d  %s  %s(%s %s) -> %s"
                      % (verdict.upper(), hit["shown"], hit["at"], hit["addr"], hit["name"],
                         hit["kind"], hit["param"],
                         ", ".join("%s*(%d)" % (k, v) for k, v in hit["casts"].most_common())))

    fixed = collections.Counter(h["reason"] for h in hits if h["reason"] is not None)

    print("%d parameter(s) cast back to a pointer in their own body: "
          "%d open (%d to one type, %d to several), %d fixed"
          % (len(hits), len(open_hits), len(one), len(several), sum(fixed.values())))

    for reason, text in REASONS:
        print("  %d %s" % (fixed[reason], text))

    return 0


if __name__ == "__main__":
    sys.exit(main())
