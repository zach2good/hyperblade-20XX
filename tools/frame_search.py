"""Search an /Od function's local names for the frame map the client has.

VC6 lays an unoptimised frame out by a hash of the local names, and declaration order settles a
shared bucket. So renaming the locals moves the slots. This reads the client's map from a target
file, probes candidate namings through tools/probe_asm.py on a scratch copy, and scores how many
locals land on the client's slot.

    python tools/frame_search.py --show                 # the map this build has now
    python tools/frame_search.py --target docs/x.txt    # score the current names against it
    python tools/frame_search.py --target docs/x.txt --search 2000
    python tools/frame_search.py --buckets iClearFace vPad pSlot

--source and --mangled pick the function; --seed fixes the random search.

A name's bucket is

    v = ((v << 2) + (v >> 4) + c) over its characters, then (v ^ (v >> 16)) & 15

Named locals land in ascending bucket order, nearest EBP first. Inside one bucket the later
declaration lands nearer EBP. So declare the locals furthest slot first and give each a name
whose bucket is no lower than the one nearer EBP. `this` and the parameters follow every local.
The hash orders one pool only: block scopes, inlined helpers, `this` and code generation
temporaries follow in that order (see docs/MATCHING.md). Probe the result with --show.

It never writes the real file, and it renames only the function's own locals, whole-word,
inside its body.
"""
import argparse
import os
import random
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# The scratch sits beside the source, so its relative includes resolve.
SCRATCH_NAME = "zzframesearch.cpp"

SYMBOL = re.compile(r"^_(\w+)\$(?:\d+)?\s*=\s*(-?\d+)", re.M)
# A code generation temporary sits inside the block; a reader that stops there loses the rest.
TEMPORARY = re.compile(r"^(\$T\d+)\s*=\s*(-?\d+)")
FRAME = re.compile(r"sub\s+esp,\s*(\d+)")

# A local renamed to one of these would rewrite a member access or a keyword in the body, so
# they never enter the search.
FORBIDDEN = set("xyzw") | {"if", "do", "for", "int"}


def bucket(name):
    """VC6's /Od frame bucket for a local's name."""
    value = 0

    for character in name:
        value = ((value << 2) + (value >> 4) + ord(character)) & 0xffffffff

    return (value ^ (value >> 16)) & 15


def listing(text, source, out):
    scratch = os.path.join(os.path.dirname(source), SCRATCH_NAME).replace("\\", "/")

    open(os.path.join(ROOT, scratch), "w", encoding="utf-8", newline="").write(text)

    # A failed probe must not leave the previous candidate's listing to be scored again.
    if os.path.exists(out):
        os.remove(out)

    try:
        done = subprocess.run([sys.executable, os.path.join("tools", "probe_asm.py"), scratch,
                               "--out", out],
                              capture_output=True, timeout=300, cwd=ROOT)

        if not os.path.exists(out):
            raise SystemExit("probe failed: %s" % done.stdout.decode("latin-1")[-400:])

        return open(out, encoding="latin-1").read()
    finally:
        path = os.path.join(ROOT, scratch)

        if os.path.exists(path):
            os.remove(path)


def frame_of(text, mangled, out, source):
    """The symbol block that sits immediately above this function's PROC line."""
    asm = listing(text, source, out)
    at = asm.find(mangled + " PROC")

    if at < 0:
        return {}, None

    head = asm[:at]

    # The block is the run of symbol lines ending at the PROC; anything above a blank run
    # belongs to the function before it.
    slots = {}

    for line in reversed(head.splitlines()):
        found = SYMBOL.match(line) or TEMPORARY.match(line)

        if found:
            slots[found.group(1)] = int(found.group(2))
        elif slots and line.strip() and not line.lstrip().startswith(";"):
            break

    size = FRAME.search(asm[at:at + 400])

    return slots, (int(size.group(1)) if size else None)


def body_span(text, signature):
    """The character range of one function's body, so a rename cannot reach past it.

    A local's name is often a member name too, so a whole-file rename would break other
    functions."""
    at = text.find(signature)

    if at < 0:
        raise SystemExit("signature not found: %s" % signature)

    open_at = text.index("{", at)
    depth = 0

    for end in range(open_at, len(text)):
        if text[end] == "{":
            depth += 1
        elif text[end] == "}":
            depth -= 1

            if depth == 0:
                return open_at, end + 1

    raise SystemExit("unbalanced body: %s" % signature)


def rename(text, mapping, span=None):
    """Every name at once, and only inside the body.

    One rule at a time would chain: a local renamed to `j` would be renamed again by the rule
    for the local called `j`."""
    if not mapping:
        return text

    pattern = r"\b(%s)\b" % "|".join(re.escape(n) for n in
                                     sorted(mapping, key=len, reverse=True))

    def swap(m):
        return mapping[m.group(1)]

    if span is None:
        return re.sub(pattern, swap, text)

    start, end = span

    return text[:start] + re.sub(pattern, swap, text[start:end]) + text[end:]


def read_target(path):
    """Lines of `slot name`, the client's map, nearest EBP first."""
    target = {}

    for line in open(path, encoding="utf-8"):
        line = line.split("#")[0].strip()

        if not line:
            continue

        slot, name = line.split()
        target[name] = int(slot)

    return target


def score(slots, target):
    return sum(1 for name, want in target.items() if slots.get(name) == want)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--source", default="src/graphics/collisioncage.cpp")
    parser.add_argument("--mangled", default="?ComputeFaceNormals@CollisionCage@@QAEXXZ")
    parser.add_argument("--target", help="file of `slot name` lines, the client's map")
    parser.add_argument("--search", type=int, default=0, help="how many candidates to try")
    parser.add_argument("--show", action="store_true", help="print the map and stop")
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--buckets", nargs="+", help="print each name's bucket and stop")
    args = parser.parse_args()

    if args.buckets:
        for name in args.buckets:
            print("   %2d  %s" % (bucket(name), name))

        return 0

    out = os.path.join(os.environ.get("TEMP", "."), "frame_search.asm")
    text = open(os.path.join(ROOT, args.source), encoding="utf-8", newline="").read()

    slots, size = frame_of(text, args.mangled, out, args.source)

    if not slots:
        sys.exit("no symbol block found for %s" % args.mangled)

    if args.show or not args.target:
        print("frame %s bytes, %d locals" % (size, len(slots)))

        for slot, name in sorted((v, k) for k, v in slots.items()):
            print("   %5d  %2d  %s" % (slot, bucket(name), name))

        return 0

    target = read_target(os.path.join(ROOT, args.target))

    print("frame %s bytes, %d locals, target names %d" % (size, len(slots), len(target)))
    print("current score %d of %d" % (score(slots, target), len(target)))

    if not args.search:
        for name in sorted(target, key=lambda n: -target[n]):
            got = slots.get(name)
            print("   %-12s want %5d  got %s  %s"
                  % (name, target[name], got, "ok" if got == target[name] else ""))

        return 0

    # The names that may move. A local the target does not mention is still renamed, because
    # it displaces the ones that are.
    movable = [n for n in slots if n not in ("this",) and "$" not in n]
    # Two letters, because a single letter collides with member names in the body. The hash table
    # sizes on the symbol count, not name length.
    pool = ["%s%s" % (a, b) for a in "abcdefghijklmnop" for b in "abcdefgh"]

    random.seed(args.seed)

    best = score(slots, target)

    print("searching %d candidates over %d names" % (args.search, len(movable)))

    for step in range(args.search):
        mapping = {}
        picks = random.sample(pool, len(movable))

        for name, pick in zip(movable, picks):
            mapping[name] = pick

        got, _ = frame_of(rename(text, mapping), args.mangled, out, args.source)

        if not got:
            continue

        # Score by where the original name's value landed, through the rename.
        back = dict((mapping.get(n, n), n) for n in movable)
        named = dict((back.get(k, k), v) for k, v in got.items())
        here = score(named, target)

        if here > best:
            best = here
            print("  %4d  score %d of %d  %s" % (step, here, len(target),
                                                 " ".join("%s=%s" % kv for kv in
                                                          sorted(mapping.items()))))

    print("best %d of %d" % (best, len(target)))

    return 0


if __name__ == "__main__":
    sys.exit(main())
