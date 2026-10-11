"""The compiler pragma state a source file puts in force at each line, for the tools that move bodies.

Only the pragmas that change what a body compiles to are tracked: optimize, auto_inline,
inline_depth, inline_recursion, function, intrinsic, check_stack, pack, code_seg and data_seg.
Lines under `#ifdef HYPERX_BRINGUP` are skipped, as the measured build skips them.

    import pragmas
    pragmas.state_at(lines, i)     # the state in force before lines[i]
    pragmas.is_pragma(line)        # a tracked pragma line
    pragmas.opens(line)            # applied to the default state, it leaves a non-default one
"""
import copy
import re

PRAGMA = re.compile(r"^\s*#\s*pragma\s+(\w+)\s*\((.*)\)")
KINDS = {"optimize", "auto_inline", "inline_depth", "inline_recursion", "function", "intrinsic",
         "check_stack", "pack", "code_seg", "data_seg"}
# The value each scalar pragma has when no pragma names it.
DEFAULTS = {"auto_inline": "on", "inline_recursion": "off", "check_stack": "on"}
# Optimize letters whose "on" is not the /O2 default.
NOT_DEFAULT_ON = set("stp")


def args_of(text):
    return [part.strip().strip('"') for part in text.split(",")]


def parse(line):
    """(kind, argument text) for a tracked pragma line, else None."""
    m = PRAGMA.match(line)
    if m and m.group(1) in KINDS:
        return m.group(1), m.group(2)
    return None


def is_pragma(line):
    return parse(line) is not None


class State:
    def __init__(self):
        self.all_off = False
        self.optimize = {}
        self.scalars = {}
        self.function = set()
        self.intrinsic = set()
        self.pack = []

    def apply(self, kind, text):
        parts = args_of(text)
        if kind == "optimize":
            switch = ""
            if len(parts) > 1:
                switch = parts[1]
            if parts[0] == "":
                self.optimize = {}
                self.all_off = switch == "off"
                return
            for flag in parts[0]:
                if switch == "on" and flag not in NOT_DEFAULT_ON:
                    self.optimize.pop(flag, None)
                else:
                    self.optimize[flag] = switch
            return
        if kind in ("function", "intrinsic"):
            names = {name for name in parts if name}
            if kind == "function":
                self.function |= names
                self.intrinsic -= names
            else:
                self.intrinsic |= names
                self.function -= names
            return
        if kind == "pack":
            if parts[0] == "push":
                self.pack.append(",".join(parts[1:]))
            elif parts[0] == "pop":
                if self.pack:
                    self.pack.pop()
            else:
                self.scalars["pack"] = parts[0]
            return
        value = ",".join(parts)
        if value == "" or DEFAULTS.get(kind) == value:
            self.scalars.pop(kind, None)
            return
        self.scalars[kind] = value

    def apply_line(self, line):
        parsed = parse(line)
        if parsed:
            self.apply(*parsed)

    def key(self):
        items = []
        if self.all_off:
            items.append('optimize("",off)')
        for flag in sorted(self.optimize):
            items.append('optimize("%s",%s)' % (flag, self.optimize[flag]))
        for kind in sorted(self.scalars):
            items.append("%s(%s)" % (kind, self.scalars[kind]))
        if self.function:
            items.append("function(%s)" % ",".join(sorted(self.function)))
        if self.intrinsic:
            items.append("intrinsic(%s)" % ",".join(sorted(self.intrinsic)))
        if self.pack:
            items.append("pack(push:%s)" % "|".join(self.pack))
        if not items:
            return "-"
        return " ".join(items)


def opens(line):
    state = State()
    state.apply_line(line)
    return state.key() != "-"


def walk(lines):
    """Yield (index, line, state before the line) for every line the measured build compiles."""
    state = State()
    skipping = []
    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith("#if"):
            skipping.append(stripped.startswith("#ifdef HYPERX_BRINGUP")
                            or stripped == "#if defined(HYPERX_BRINGUP)")
            continue
        if stripped.startswith("#else") and skipping:
            skipping[-1] = not skipping[-1]
            continue
        if stripped.startswith("#endif") and skipping:
            skipping.pop()
            continue
        if any(skipping):
            continue
        yield i, line, state
        state.apply_line(line)


def state_before(lines, index):
    """A copy of the state in force before lines[index]; index may be len(lines)."""
    state = State()
    for i, line, state in walk(lines):
        if i >= index:
            return copy.deepcopy(state)
    return copy.deepcopy(state)


def state_at(lines, index):
    """The state key in force before lines[index]; index may be len(lines)."""
    return state_before(lines, index).key()


def blank(line):
    return line.strip() == ""


class Ambiguous(Exception):
    pass


def wrap(lines, first, last):
    """The lines to carry with the body at lines[first:last], as (first, last, outside state key).

    first is the top of the body's comment block and last is one past its closing brace. The
    pragmas directly above that open state, and the ones directly below that close it again,
    move with the body. Raises Ambiguous when the body's state reaches past it: a region that
    also covers a neighbour, or an opener below a closer in the run above.
    """
    run = []
    i = first - 1
    while i >= 0 and (blank(lines[i]) or is_pragma(lines[i])):
        if is_pragma(lines[i]):
            run.insert(0, i)
        i -= 1
    openers = []
    for k in run:
        if opens(lines[k]):
            openers.append(k)
        elif openers:
            raise Ambiguous("a closing pragma sits between the opening ones above the body")
    top = first
    if openers:
        top = openers[0]
    outside = state_before(lines, top)
    inside = state_before(lines, first)
    for k in range(first, last):
        inside.apply_line(lines[k])
    bottom = last
    j = last
    while inside.key() != outside.key() and j < len(lines) and (blank(lines[j]) or is_pragma(lines[j])):
        inside.apply_line(lines[j])
        j += 1
        if is_pragma(lines[j - 1]):
            bottom = j
    if inside.key() != outside.key():
        raise Ambiguous("the pragma state around the body [%s] reaches past it; outside it is [%s]"
                        % (inside.key(), outside.key()))
    if openers and bottom == last:
        raise Ambiguous("the pragmas above the body open nothing that closes below it")
    return top, bottom, outside.key()


def insertion(lines, at):
    """Move an insertion point at the top of a body's comment block above that body's openers."""
    k = at - 1
    top = at
    while k >= 0 and (blank(lines[k]) or is_pragma(lines[k])):
        if is_pragma(lines[k]):
            if not opens(lines[k]):
                break
            top = k
        k -= 1
    return top
