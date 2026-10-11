"""Name the bodies a move, merge or sort put under different pragmas from the ones they had.

A body mover that lifts the text between a marker and its closing brace leaves the pragma lines
around it behind, and the body then compiles under the target file's switches. This walks
commits and, for every marked body whose file or neighbours changed, compares the pragma state
in force at its marker before and after. The state is read from the file alone: optimize,
auto_inline, inline_depth, inline_recursion, function, intrinsic, check_stack, pack, code_seg
and data_seg. Pragmas under `#ifdef HYPERX_BRINGUP` are skipped, as the measured build skips them.

    python tools/lost_pragmas.py                 # every commit under src
    python tools/lost_pragmas.py e4281b59 ...    # only these commits
    python tools/lost_pragmas.py --open          # only the LOST bodies
    python tools/lost_pragmas.py --unmoved       # bodies whose state changed where they stood

Each line gives the commit, the address, the name, the state before and after, and the state
HEAD compiles the body under. A body is LOST when the move dropped a pragma it had and HEAD
still lacks it. A body that gained a pragma is listed without a flag, since folds add them on
purpose. Exit code 1 when any body is LOST.
"""
import re
import subprocess
import sys

import pragmas

MARKER = re.compile(r"^//\s*FUNCTION:\s*HYPERX\s+(0x[0-9a-fA-F]+)")
NULL_BLOB = "0" * 40


def bodies(text):
    """Address -> (state key, name) for every marked body, and address -> its two neighbours."""
    found = {}
    order = []
    lines = text.replace("\r\n", "\n").split("\n")
    for i, line, state in pragmas.walk(lines):
        m = MARKER.match(line)
        if not m:
            continue
        address = int(m.group(1), 16)
        name = ""
        for follow in lines[i + 1:i + 4]:
            if "(" in follow and not follow.startswith("//"):
                name = follow.split("(")[0].split()[-1]
                break
        found[address] = (state.key(), name)
        order.append(address)
    neighbours = {}
    for k, address in enumerate(order):
        before = None
        after = None
        if k > 0:
            before = order[k - 1]
        if k + 1 < len(order):
            after = order[k + 1]
        neighbours[address] = (before, after)
    return found, neighbours


class Blobs:
    def __init__(self):
        self.proc = subprocess.Popen(["git", "cat-file", "--batch"], stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE)
        self.cache = {}

    def text(self, sha):
        if sha in self.cache:
            return self.cache[sha]
        self.proc.stdin.write((sha + "\n").encode())
        self.proc.stdin.flush()
        header = self.proc.stdout.readline().split()
        size = int(header[2])
        data = self.proc.stdout.read(size + 1)[:size]
        result = bodies(data.decode("latin1"))
        if len(self.cache) < 20000:
            self.cache[sha] = result
        return result


def git(*args):
    return subprocess.run(["git"] + list(args), capture_output=True, text=True,
                          errors="replace").stdout


def commits(revs):
    """[(sha, title, [(old blob, new blob, path)])] for each non-merge commit touching src."""
    cmd = ["log", "--no-merges", "--no-renames", "--raw", "--no-abbrev", "--format=@%H %s"]
    if revs:
        cmd += ["--no-walk"] + revs
    cmd += ["--", "src"]
    out = []
    for line in git(*cmd).split("\n"):
        if line.startswith("@"):
            sha, title = (line[1:] + " ").split(" ", 1)
            out.append((sha, title.strip(), []))
        elif line.startswith(":") and out:
            meta, path = line.split("\t", 1)
            fields = meta.split()
            if path.endswith((".cpp", ".h", ".inl")):
                out[-1][2].append((fields[2], fields[3], path))
    return out


def head_states(blobs):
    states = {}
    for line in git("ls-tree", "-r", "HEAD", "src").split("\n"):
        if "\t" not in line:
            continue
        meta, path = line.split("\t", 1)
        if not path.endswith((".cpp", ".h", ".inl")):
            continue
        found, unused = blobs.text(meta.split()[2])
        for address, (key, name) in found.items():
            states[address] = (key, path)
    return states


def side(blobs, changes, which):
    found = {}
    near = {}
    for change in changes:
        sha = change[which]
        if sha == NULL_BLOB:
            continue
        states, neighbours = blobs.text(sha)
        for address, (key, name) in states.items():
            found[address] = (key, name, change[2])
        near.update(neighbours)
    return found, near


def main():
    args = sys.argv[1:]
    only_open = "--open" in args
    unmoved = "--unmoved" in args
    revs = [a for a in args if a not in ("--open", "--unmoved")]
    blobs = Blobs()
    head = head_states(blobs)
    opened = 0
    for sha, title, changes in commits(revs):
        before, near_before = side(blobs, changes, 0)
        after, near_after = side(blobs, changes, 1)
        for address in sorted(set(before) & set(after)):
            old_key, name, old_path = before[address]
            new_key, unused, new_path = after[address]
            if old_key == new_key:
                continue
            moved = old_path != new_path or near_before[address] != near_after[address]
            if moved == unmoved:
                continue
            head_key, head_path = head.get(address, ("(gone)", ""))
            dropped = set(old_key.split()) - set(new_key.split()) - {"-"}
            is_open = bool(dropped) and head_key != "(gone)" and not dropped <= set(head_key.split())
            if is_open:
                opened += 1
            if only_open and not is_open:
                continue
            print("%s 0x%x %s  %s  [%s] -> [%s]  HEAD [%s]%s  %s" % (
                sha[:8], address, name, old_path + ("" if old_path == new_path else " -> " + new_path),
                old_key, new_key, head_key, " LOST" if is_open else "", title[:60]))
    print("%d lost" % opened)
    return 1 if opened else 0


sys.exit(main())
