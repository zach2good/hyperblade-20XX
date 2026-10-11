"""Find a note above a marked body that describes a different body.

    python tools/misplaced_notes.py                # the suspects, strongest first
    python tools/misplaced_notes.py --why          # also the terms each score rests on
    python tools/misplaced_notes.py --root DIR     # read DIR/src, such as an old tree from git archive

A sort or a fold moves bodies and leaves their notes behind, so a paragraph ends up above a
function it does not describe. check_self_naming_notes.py catches only a note that names its own
file; this reads what a note mentions and asks which body holds it.

Each paragraph above a FUNCTION marker, back to the previous code line, is scored against every
marked body. A term counts for a body when the body's code holds it: an identifier, a number, a
string literal, or a word that is part of an identifier (the "flip" in DDraw::Flip). Rare terms
weigh more than common ones, and code terms more than words. A paragraph is a suspect when
another body holds much more of it than the body below it does.

It also lists each note left at the end of a file, below the last line of code, which describes
nothing.

A suspect is a claim. A note about its body in general terms, or about a callee, can score
higher on the callee. Read the note against both bodies before moving it. Of the 62 suspects
this listed on 27 September 2026 about half were real; a note in plain words with no code term
is often missed, so a file with one real suspect is worth reading end to end.

Read only. Exits 0.
"""
import collections
import glob
import math
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SKIP = ("jpeg/",)

MARKER = re.compile(r"^\s*//\s*(FUNCTION|LIBRARY|SYNTHETIC|TEMPLATE|STRING|GLOBAL|VTABLE):"
                    r"\s*HYPERX\s+(0x[0-9a-fA-F]+)")
COMMENT = re.compile(r"^\s*//(.*)$")
IDENT = re.compile(r"(?<![0-9A-Za-z_])[A-Za-z_][A-Za-z0-9_]*")
NUMBER = re.compile(r"(?<![\w.])(0[xX][0-9a-fA-F]+|\d+)(?![\w.])")
LITERAL = re.compile(r'"((?:\\.|[^"\\\n])*)"')
PART = re.compile(r"[A-Z]+(?=[A-Z][a-z])|[A-Z]?[a-z]+|[A-Z]+")
DECLARED = re.compile(r"(~?[A-Za-z_]\w*(?:\s*::\s*~?[A-Za-z_]\w*)*)\s*\(")

KEYWORDS = set("""
    if else for while do switch case default break continue return goto sizeof void int char
    short long float double unsigned signed const static extern struct class union enum typedef
    public private protected virtual inline this new delete operator true false bool namespace
    using template typename register volatile __cdecl __stdcall __fastcall __thiscall NULL
""".split())

STOPWORDS = set("""
    that this with from what when which where there their them they then than these those have
    been being were will would could should must also only just into onto over under same
    each every other another some such more most less least much many does done doing both
    either neither here once twice after before while because since though although through
    across about above below between within without again still even never always first
    second third last next like make makes made take takes took gives give given keeps keep
    kept reads read writes write wrote holds hold held says said call calls called body bodies
    function functions note notes original originals compiler compiled code line lines byte
    bytes spelling spellings score match matches matched shape instead whole part parts
    rather thing things value values stays stay left right word words sits your type types
    name names named case cases form forms means mean used uses using none zero pointer
    return returns null local result
    """.split())

# Code terms are the stronger evidence: a word can be an accident of phrasing, a field name
# rarely is.
CODE = 3.0
WORD = 1.0

# A paragraph is a suspect when another body outscores its own by this factor and this margin.
RATIO = 2.5
MARGIN = 8.0

# Most stranded notes moved one or two bodies along in their own file, so a rival in another file
# needs more evidence.
ABROAD = 0.6

# The rival must share at least this many terms with the note.
HITS = 3

# BM25's length normalisation, with a term counted once per body.
SATURATION = 1.2
LENGTH = 0.75

# Numbers below this are mostly field offsets and loop counts, and every blob body has them.
SMALL = 0x100


def stem(word):
    word = word.lower()

    if len(word) > 4 and word.endswith("ies"):
        return word[:-3] + "y"

    if len(word) > 4 and word.endswith("s") and not word.endswith("ss"):
        return word[:-1]

    return word


def parts(identifier):
    """The words an identifier is built from, four letters or longer."""
    found = set()

    for piece in PART.findall(identifier):
        if len(piece) >= 4:
            found.add(stem(piece))

    return found


def number(text):
    try:
        return int(text, 16) if text.lower().startswith("0x") else int(text)
    except ValueError:
        return None


def strip_code(text):
    """The code with comments blanked and string literals pulled out."""
    out = []
    literals = []
    at = 0
    size = len(text)

    while at < size:
        c = text[at]

        if text.startswith("//", at):
            end = text.find("\n", at)
            at = size if end < 0 else end
            continue

        if text.startswith("/*", at):
            end = text.find("*/", at + 2)
            at = size if end < 0 else end + 2
            out.append(" ")
            continue

        if c == '"' or c == "'":
            end = at + 1

            while end < size and text[end] != c and text[end] != "\n":
                end += 2 if text[end] == "\\" else 1

            if c == '"':
                literals.append(text[at + 1:end])

            out.append(" ")
            at = end + 1
            continue

        out.append(c)
        at += 1

    return "".join(out), literals


def body_end(lines, start):
    """The index of the line that closes the body declared at `start`, or None for a prototype."""
    depth = 0
    opened = False
    limit = min(len(lines), start + 4000)

    for index in range(start, limit):
        code, unused = strip_code(lines[index])

        for c in code:
            if c == ";" and not opened and depth == 0:
                return None

            if c == "{":
                depth += 1
                opened = True
            elif c == "}":
                depth -= 1

                if opened and depth == 0:
                    return index

    return None


class Body:
    def __init__(self, path, line, address, name, code, literals):
        self.path = path
        self.line = line
        self.address = address
        self.name = name
        self.terms = set()

        for identifier in IDENT.findall(code):
            if identifier in KEYWORDS:
                continue

            self.terms.add(("id", identifier))

            for word in parts(identifier):
                self.terms.add(("word", word))

        for text in NUMBER.findall(code):
            value = number(text)

            if value is not None and value >= SMALL:
                self.terms.add(("num", value))

        for literal in literals:
            if literal:
                self.terms.add(("str", literal))


class Note:
    def __init__(self, path, line, text, owner):
        self.path = path
        self.line = line
        self.text = text
        self.owner = owner


def note_terms(text, identifiers, addresses):
    """The terms a note mentions that some body could hold."""
    terms = set()
    starts = set(match.start(1) for match in re.finditer(r"(?:^|[.!?:]\s+)(\w)", text))

    for match in IDENT.finditer(text):
        word = match.group(0)
        coded = ("_" in word or re.search(r"[a-z][A-Z]", word) or re.search(r"[A-Za-z]\d", word)
                 or (word[0].isupper() and match.start() not in starts))

        if coded:
            if word in identifiers and word not in KEYWORDS:
                terms.add(("id", word))

            for piece in parts(word):
                terms.add(("word", piece))

            continue

        if word.islower() and len(word) >= 4 and word not in STOPWORDS:
            terms.add(("word", stem(word)))

    for text_number in NUMBER.findall(text):
        if not text_number.lower().startswith("0x") and len(text_number) < 2:
            continue

        value = number(text_number)

        if value is None or value < SMALL:
            continue

        if value in addresses:
            name = addresses[value]
            terms.add(("id", name.split("::")[-1]))
            continue

        terms.add(("num", value))

    for literal in LITERAL.findall(text):
        terms.add(("str", literal))

    return terms


def paragraphs(lines, marker, floor):
    """The comment paragraphs above the marker at `marker`, nearest first, each (line, text)."""
    found = []
    index = marker - 1
    block = []

    while index > floor:
        line = lines[index]

        if MARKER.match(line):
            # Two markers stacked on one body, or a GLOBAL just above: stop at anything that is
            # not this body's own FUNCTION marker.
            if not line.strip().startswith("// FUNCTION"):
                break

            index -= 1
            continue

        comment = COMMENT.match(line)

        if comment:
            block.append(comment.group(1).strip())
            index -= 1
            continue

        if not line.strip():
            if block:
                found.append((index + 2, " ".join(reversed(block))))
                block = []

            index -= 1
            continue

        break

    if block:
        found.append((index + 2, " ".join(reversed(block))))

    return found


def read_tree(root):
    bodies = []
    notes = []

    for suffix in ("*.cpp", "*.h"):
        for path in sorted(glob.glob(os.path.join(root, "src", "**", suffix), recursive=True)):
            shown = os.path.relpath(path, os.path.join(root, "src")).replace(os.sep, "/")

            if shown.startswith(SKIP):
                continue

            lines = open(path, encoding="utf-8", errors="replace").read().splitlines()
            floor = -1
            index = 0

            while index < len(lines):
                marker = MARKER.match(lines[index])

                if not marker or marker.group(1) != "FUNCTION":
                    index += 1
                    continue

                start = index
                declared = index + 1

                while declared < len(lines) and MARKER.match(lines[declared]):
                    declared += 1

                end = body_end(lines, declared)

                if end is None:
                    index = declared
                    continue

                code, literals = strip_code("\n".join(lines[declared:end + 1]))
                found = DECLARED.search(code)
                name = re.sub(r"\s+", "", found.group(1)) if found else "?"
                body = Body("src/" + shown, declared + 1, int(marker.group(2), 16), name, code,
                            literals)
                bodies.append(body)

                for line, text in paragraphs(lines, start, floor):
                    notes.append(Note("src/" + shown, line, text, body))

                floor = end
                index = end + 1

    return bodies, notes


def stranded(root):
    """Each note below the last line of code in its file, as (path, line, text)."""
    found = []

    for suffix in ("*.cpp", "*.h"):
        for path in sorted(glob.glob(os.path.join(root, "src", "**", suffix), recursive=True)):
            shown = os.path.relpath(path, os.path.join(root, "src")).replace(os.sep, "/")

            if shown.startswith(SKIP):
                continue

            lines = open(path, encoding="utf-8", errors="replace").read().splitlines()
            index = len(lines) - 1

            while index >= 0 and (not lines[index].strip() or COMMENT.match(lines[index])):
                index -= 1

            block = [line.strip()[2:].strip() for line in lines[index + 1:] if COMMENT.match(line)]

            if block:
                first = index + 1

                while not COMMENT.match(lines[first]):
                    first += 1

                found.append(("src/" + shown, first + 1, " ".join(block)))

    return found


def weights(bodies):
    """Each term's weight, from how few bodies hold it."""
    frequency = collections.Counter()

    for body in bodies:
        frequency.update(body.terms)

    total = float(len(bodies))
    weight = {}

    for term, count in frequency.items():
        kind = WORD if term[0] == "word" else CODE
        weight[term] = kind * math.log(total / count)

    return weight


def find(root):
    """Each suspect as (note, the three best rivals, own score, best score, terms)."""
    bodies, notes = read_tree(root)
    weight = weights(bodies)
    holders = collections.defaultdict(list)
    size = {}

    for body in bodies:
        size[body] = len(body.terms)

        for term in body.terms:
            holders[term].append(body)

    average = sum(size.values()) / float(len(bodies))
    identifiers = set(term[1] for term in weight if term[0] == "id")
    functions = set(body.name.split("::")[-1] for body in bodies)
    addresses = dict((body.address, body.name) for body in bodies)

    def scale(body):
        # A long body holds a little of everything, so a term found in it counts for less.
        return (SATURATION + 1.0) / (1.0 + SATURATION * (1.0 - LENGTH + LENGTH * size[body] / average))

    found = []

    for note in notes:
        terms = set(term for term in note_terms(note.text, identifiers, addresses)
                    if term in weight and not (term[0] == "id" and term[1] in functions))

        if not terms:
            continue

        own = scale(note.owner) * sum(weight[term] for term in terms if term in note.owner.terms)
        rivals = collections.Counter()
        hits = collections.Counter()

        for term in terms:
            for body in holders[term]:
                if body is not note.owner:
                    rivals[body] += weight[term]
                    hits[body] += 1

        ranked = []

        for body, total in rivals.items():
            if hits[body] < HITS:
                continue

            total *= scale(body)

            if body.path != note.path:
                total *= ABROAD

            ranked.append((total, body))

        ranked.sort(key=lambda row: -row[0])

        if ranked and ranked[0][0] >= MARGIN + own and ranked[0][0] >= RATIO * own:
            found.append((note, ranked[:3], own, ranked[0][0], terms))

    found.sort(key=lambda row: row[2] - row[3])

    return found, notes, bodies, weight


def main():
    args = sys.argv[1:]
    root = ROOT
    why = "--why" in args

    if "--root" in args:
        root = os.path.abspath(args[args.index("--root") + 1])

    suspects, notes, bodies, weight = find(root)

    for note, homes, own, top, terms in suspects:
        best = homes[0][1]
        print("%s:%d  above %s, own %.0f" % (note.path, note.line, note.owner.name, own))
        print("    %s" % note.text[:160])

        for total, body in homes:
            print("    home? %s %.0f at %s:%d" % (body.name, total, body.path, body.line))

        if why:
            held = sorted(terms & best.terms - note.owner.terms, key=lambda term: -weight[term])
            print("    only there: %s" % ", ".join(str(term[1]) for term in held[:12]))

    tails = stranded(root)

    for path, line, text in tails:
        print("%s:%d  at the end of the file" % (path, line))
        print("    %s" % text[:160])

    print("%d suspect note(s) of %d paragraphs above %d marked bodies, and %d at the end of a file"
          % (len(suspects), len(notes), len(bodies), len(tails)))

    return 0


if __name__ == "__main__":
    sys.exit(main())
