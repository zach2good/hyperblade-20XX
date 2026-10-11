"""Print a compiled plan file as indented pseudo-source, every id named from ENUMIDS.H.

    python tools/plan_dump.py CARRYROK                 # one plan, by name or path
    python tools/plan_dump.py                          # every plan under PLANS
    python tools/plan_dump.py OFFWING --data D:/Data   # the game's Data folder
    python tools/plan_dump.py --decisions              # handlers a network replay misreads

The parser is plan_graph.py's. This adds the reading: a node's leaves become its call's
arguments and its child nodes its body. A DECIDE arm is numbered by the value the decision
function returns to pick it, so a DECIDE_BY_WITH_AMONG's arms start at one: slot nought is its
DataBlock, the decision's arguments. A Block is flattened into its arm.

A leaf prints as the developers' name where its position has one. Otherwise 0xfedcNNNN is
pronoun NNNN, 0xedcbNNNN is local real NNNN, 0xdcbaNNNN is interactor parameter NNNN, a
compiler token prints as its token, a word with a float's bits prints with a decimal point
(the compiler stores a number written with one as a float), and anything else large prints in
hex.

--decisions walks each handler as ExecuteScript does, packs every path's decision log, keeps the
bits the network keeps (14 for a message, 8 for a collision) and replays it. It lists handlers
with three or more DECIDEs on a path, a second branch of 32 or more, or a collision rule with any
DECIDE, and says where the replay leaves the local path. --selftest checks the emulation.

--kinds adds a comment to two argument-kind traps: a SET_TIMEOUTMSG with no fourth leaf, whose
message then carries data 0, and a pronoun naming a slot rather than a collision object in a
SET_COLLISION_INTERACTOR. The GOALIE plan names DecideWithRating's rating from GoalieRatingsID,
the table a goalie's ratings are stored in.

Reads PLANS/*.PBN and ENUMIDS.H from --data, else HB_DATA, else build/out/Data. Exits 1
naming the path when either is missing or a plan does not parse.
"""

import argparse
import glob
import os
import struct
import sys

import plan_graph

SELECTOR = 0xFEDC
LOCAL_REAL = 0xEDCB
INTERACTOR_PARAM = 0xDCBA

GLOBALS = 0x001B0000
DECLARE = 0x00070000
CHANGE = 0x00070002
PLAN_CHANGE = 0x00160000
DECIDES = (0x00170000, 0x00170001)
BLOCK = 0x40000000
DATA_BLOCK = 0x20000000
COLLISION = (0x00030000, 0x00030001)
TIMEOUT = 0x001A0002

# A collision interactor's leaves are collision objects; these pronoun names mark the ones that are.
COLLISION_KINDS = ('COB', 'Bub', 'Side', 'Zone')

# The enum that names the leaf at each child position of an opcode.
POSITION_ENUM = dict(plan_graph.ARGUMENT_ENUM)
POSITION_ENUM[0x00070003] = {0: 'BehaviorSetParamFnIDs'}
POSITION_ENUM[0x00180001] = {0: 'InteractorSetParamFnIDs'}

# Action arguments with an enum, by child position. IncrementStat counts an id_STA_ match stat.
ACTION_ARGUMENT_ENUM = {'id_ACF_IncrementStat': {2: 'StatID'}}

for _action, _position in plan_graph.MESSAGE_ACTIONS.items():
    ACTION_ARGUMENT_ENUM[_action] = {_position: 'ID_MSG'}

# Decision arguments with an enum, by DataBlock position. DecideWithRating reads a rating id.
DECISION_ARGUMENT_ENUM = {'id_DCF_DecideWithRating': {0: 'RatingMeasureID'}}


def number(value):
    signed = value - (1 << 32) if value & 0x80000000 else value

    if -0x10000 < signed < 0x800000:
        return '%d' % signed

    real = struct.unpack('<f', struct.pack('<I', value))[0]
    text = '%g' % real

    if real == real and abs(real) != float('inf') and struct.pack('<f', float(text)) == struct.pack('<I', value):
        if '.' not in text and 'e' not in text:
            text = text + '.0'

        return text

    return '0x%08x' % value


class Dumper:
    def __init__(self, tables, plans_dir, out, kinds=False):
        self.tables = tables
        self.kinds = kinds
        self.plan = ''
        self.plans_dir = plans_dir
        self.out = out
        self.names = []
        self.declared = 0
        self.foreign = {}

    def enum(self, table, value):
        rows = self.tables.get(table) or []

        if 0 <= value < len(rows):
            return rows[value]

        return '%s?%s' % (table, number(value))

    def leaf(self, value, table=None):
        if value in plan_graph.SENTINELS:
            return plan_graph.SENTINELS[value]

        high = value >> 16
        low = value & 0xffff

        if high == SELECTOR:
            return self.enum('Pronouns', low)

        if high == LOCAL_REAL:
            return 'local[%d]' % low

        if high == INTERACTOR_PARAM:
            return 'param[%d]' % low

        if table:
            return self.enum(table, value)

        return number(value)

    def behaviour(self, value, names):
        if value in plan_graph.SENTINELS:
            return plan_graph.SENTINELS[value]

        if 0 <= value < len(names):
            return '%d "%s"' % (value, names[value])

        return number(value)

    def plan_behaviours(self, plan):
        """The behaviour names of another plan, for CHANGE_OF_PLAN's second argument."""
        key = plan.upper()

        if key not in self.foreign:
            path = os.path.join(self.plans_dir, key + '.PBN')
            found = []

            if os.path.exists(path):
                found = plan_graph.behaviours(plan_graph.parse(plan_graph.read(path)))

            self.foreign[key] = found

        return self.foreign[key]

    def arguments(self, opcode, children, action):
        """The leaves of a node as call arguments, each named where its position says how."""
        texts = []
        positions = dict(POSITION_ENUM.get(opcode, {}))
        positions.update(ACTION_ARGUMENT_ENUM.get(action, {}))

        for index in range(len(children)):
            kind, value, _ = children[index]

            if kind == 'str':
                texts.append('"%s"' % value)
            elif kind == 'leaf':
                texts.append(self.leaf(value, positions.get(index)))

        return texts

    def line(self, depth, text):
        self.out.write('    ' * depth + text + '\n')

    def body(self, children, depth):
        for child in children:
            if child[0] == 'node' or child[0] == 'end':
                self.node(child, depth)

    def decide(self, opcode, children, depth):
        function = self.leaf(children[0][1], 'DecisionFunctionIDs')
        arguments = []
        first_arm = 1

        if opcode == DECIDES[1] and len(children) > 1 and children[1][1] == DATA_BLOCK:
            enums = dict(DECISION_ARGUMENT_ENUM.get(function, {}))

            if self.plan == 'GOALIE.PBN' and 0 in enums:
                enums[0] = 'GoalieRatingsID'

            leaves = [c for c in children[1][2] if c[0] == 'leaf']
            arguments = [self.leaf(leaves[i][1], enums.get(i)) for i in range(len(leaves))]
            first_arm = 2

        keyword = plan_graph.OPCODES[opcode]
        self.line(depth, '%s %s(%s)' % (keyword, function, ', '.join(arguments)))

        for position in range(first_arm, len(children)):
            arm = children[position]
            label = '[%d]' % (position - 1)

            if arm[0] != 'node':
                self.line(depth + 1, '%s %s' % (label, self.leaf(arm[1])))
            elif arm[1] == BLOCK:
                self.line(depth + 1, label)
                self.body(arm[2], depth + 2)
            else:
                self.node(arm, depth + 1, label + ' ')

    def node(self, node, depth, prefix=''):
        kind, opcode, children = node

        if kind == 'end':
            self.line(depth, prefix + 'END_PLAN')
            return

        keyword = plan_graph.OPCODES.get(opcode, 'op_%04x_%04x' % (opcode >> 16, opcode & 0xffff))

        if opcode in DECIDES and children and children[0][0] == 'leaf':
            if prefix:
                self.line(depth, prefix.strip())
                depth = depth + 1

            self.decide(opcode, children, depth)
            return

        if opcode == DECLARE:
            self.out.write('\n')
            self.line(depth, prefix + 'DECLARE_BEHAVIOR %d "%s" %s'
                      % (self.declared, children[1][1], self.enum('Behavior_IDs', children[0][1])))
            self.declared = self.declared + 1
            self.body(children[2:], depth + 1)
            return

        if opcode == GLOBALS:
            self.line(depth, prefix + keyword)
            self.body(children, depth + 1)
            return

        if opcode == CHANGE:
            self.line(depth, prefix + '%s %s' % (keyword, self.behaviour(children[0][1], self.names)))
            return

        if opcode == PLAN_CHANGE and len(children) == 2 and children[0][0] == 'str':
            target = self.behaviour(children[1][1], self.plan_behaviours(children[0][1]))
            self.line(depth, prefix + '%s "%s" %s' % (keyword, children[0][1], target))
            return

        action = ''

        if opcode in plan_graph.ACTIONS:
            action = plan_graph.action_of(self.tables, children)

        arguments = self.arguments(opcode, children, action)

        if opcode in plan_graph.ACTIONS and arguments:
            text = '%s %s(%s)' % (keyword, arguments[0], ', '.join(arguments[1:]))
        elif arguments:
            text = '%s %s' % (keyword, ', '.join(arguments))
        else:
            text = keyword

        self.line(depth, prefix + text + self.kind_note(opcode, arguments))
        self.body(children, depth + 1)

    def kind_note(self, opcode, arguments):
        if not self.kinds:
            return ''

        if opcode == TIMEOUT and len(arguments) == 3:
            return '  // data 0'

        if opcode in COLLISION:
            slots = [a for a in arguments
                     if a.startswith('id_PRN_') and not any(k in a for k in COLLISION_KINDS)]

            if slots:
                return '  // slot pronoun as a collision object: %s' % ', '.join(slots)

        return ''

    def dump(self, path):
        tops = plan_graph.parse(plan_graph.read(path))
        self.names = plan_graph.behaviours(tops)
        self.plan = os.path.basename(path).upper()
        self.declared = 0
        self.line(0, '// %s' % os.path.basename(path))

        for top in tops:
            self.node(top, 0)


def short(value):
    value = value & 0xffff

    if value & 0x8000:
        return value - 0x10000

    return value


def push(word, branch):
    """DecisionLog::Push, src/planvm/decisionlog.h, faults included."""
    if word == -1:
        word = 0

    if word & 0x3000 == 0:
        return short((word & 0xfff) | branch | 0x9000)

    if word & 0x3000 == 0x1000:
        return short((branch << 7) | (word & 0x7f) | 0xa000)

    return short(((word >> 2) & 0x1e0) | (word & 0x1f) | (branch & 0xe00) | 0xb000)


def pop(word):
    """DecisionLog::Pop: the oldest branch and the word left behind."""
    if word & 0x3000 == 0x1000:
        return word & 0xfff, 0

    if word & 0x3000 == 0x2000:
        return word & 0x7f, short(((word >> 7) & ~0x1e0) | 0x1000)

    return word & 0x1f, short((short(((word >> 3) & 0x3c) | (word & 0xe00)) >> 2) | 0x2000)


def arms(decide):
    """A DECIDE's arms as (branch, node), numbered as ExecuteScript indexes them."""
    found = []

    for position in range(1, len(decide[2])):
        found.append((position - 1, decide[2][position]))

    return found


def first_arm(decide):
    if decide[1] == DECIDES[1] and len(decide[2]) > 1 and decide[2][1][1] == DATA_BLOCK:
        return 1

    return 0


# A handler with more paths than this is reported as too wide rather than walked.
PATH_LIMIT = 4096


def local_paths(nodes):
    """Every sequence of (DECIDE, branch) a recording run can take through nodes, in run order."""
    paths = [[]]

    for node in nodes:
        if node[0] != 'node':
            continue

        if node[1] in DECIDES:
            choices = []

            for branch, arm in arms(node)[first_arm(node):]:
                for tail in local_paths([arm]):
                    choices.append([(node, branch)] + tail)

            paths = [p + c for p in paths for c in choices]
        elif node[1] == BLOCK:
            paths = [p + t for p in paths for t in local_paths(node[2])]

        if len(paths) > PATH_LIMIT:
            raise OverflowError

    return paths


def replay(nodes, word, taken):
    """Runs nodes as a network copy does, popping each branch from word; returns the word left."""
    for node in nodes:
        if node[0] != 'node':
            continue

        if node[1] in DECIDES:
            branch, word = pop(word)
            taken.append((node, branch))

            if len(node[2]) - 1 > branch:
                arm = node[2][1 + branch]

                if arm[0] != 'node' or arm[1] == DATA_BLOCK:
                    taken.append((None, 'throws: arm %d is not an instruction' % branch))
                    return word

                word = replay([arm], word, taken)
        elif node[1] == BLOCK:
            word = replay(node[2], word, taken)

    return word


def handlers(tops, names):
    """Every interactor with an Enable block, as (place, interactor node, script)."""
    found = []
    behaviour = 0

    for top in tops:
        if top[0] != 'node':
            continue

        if top[1] == GLOBALS:
            place = 'global'
        elif top[1] == DECLARE:
            place = '%d "%s"' % (behaviour, names[behaviour])
            behaviour = behaviour + 1
        else:
            continue

        for child in top[2]:
            if child[0] != 'node' or child[1] >> 16 not in (0x0003, 0x0005):
                continue

            for item in child[2]:
                if item[0] == 'node' and item[1] == 0x80000000:
                    found.append((place, child, item[2]))

    return found


def selftest():
    one = push(-1, 5)
    assert pop(one & 0x3fff) == (5, 0)

    two = push(push(-1, 3), 31)
    branch, rest = pop(two & 0x3fff)
    assert (branch, pop(rest)[0]) == (3, 31)

    # A second branch of 32 spills into the count, so the replay reads three.
    assert push(push(-1, 3), 32) & 0x3000 == 0x3000

    # The third branch is masked with 0xe00, so anything under 0x200 replays as nought.
    three = push(push(push(-1, 1), 2), 3)
    first, rest = pop(three & 0x3fff)
    second, rest = pop(rest)
    assert (first, second, pop(rest)[0]) == (1, 2, 0)

    # A collision keeps eight bits: the count reads nought, the three-branch arm takes five bits.
    assert pop(push(-1, 37) & 0xff)[0] == 5
    print('selftest passed')
    return 0


class Decisions:
    """The --decisions report: handlers whose network replay leaves the local object's path."""

    def __init__(self, dumper, out):
        self.dumper = dumper
        self.out = out

    def label(self, decide):
        if decide is None:
            return ''

        return self.dumper.leaf(decide[2][0][1], 'DecisionFunctionIDs')

    def steps(self, taken):
        texts = []

        for decide, branch in taken:
            if decide is None:
                texts.append(branch)
            else:
                texts.append('%s[%d]' % (self.label(decide), branch))

        return ' > '.join(texts) or '(none)'

    def report(self, path):
        tops = plan_graph.parse(plan_graph.read(path))
        names = plan_graph.behaviours(tops)
        plan = os.path.basename(path).upper()

        for place, interactor, script in handlers(tops, names):
            collision = interactor[1] >> 16 == 0x0003

            try:
                paths = local_paths(script)
            except OverflowError:
                self.out.write('%s %s: more than %d paths, not walked\n' % (plan, place, PATH_LIMIT))
                continue

            longest = max(len(p) for p in paths)

            if longest == 0:
                continue

            second_wide = any(len(p) > 1 and p[1][1] >= 32 for p in paths)

            if longest < 3 and not second_wide and not collision:
                continue

            keyword = plan_graph.OPCODES.get(interactor[1])
            arguments = self.dumper.arguments(interactor[1], interactor[2], '')
            self.out.write('%s %s %s %s\n' % (plan, place, keyword, ', '.join(arguments)))

            for local in paths:
                word = -1

                for _, branch in local:
                    word = push(word, branch)

                mask = 0xff if collision else 0x3fff
                taken = []
                replay(script, short(word & mask), taken)

                if taken == local:
                    verdict = 'same'
                else:
                    verdict = 'replay ' + self.steps(taken)

                self.out.write('    %s  ->  %s\n' % (self.steps(local), verdict))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('plans', nargs='*', help='plan names or .PBN paths; every plan when none')
    parser.add_argument('--data', help='the game Data folder holding PLANS and ENUMIDS.H')
    parser.add_argument('--kinds', action='store_true',
                        help='comment timers that carry data 0 and slot pronouns used as collision objects')
    parser.add_argument('--decisions', action='store_true',
                        help='list handlers a network replay of the decision log can misread')
    parser.add_argument('--selftest', action='store_true', help='check the decision log emulation')
    options = parser.parse_args()

    if options.selftest:
        return selftest()

    data = options.data or os.environ.get('HB_DATA') or os.path.join(plan_graph.ROOT, 'build', 'out', 'Data')
    plans_dir = os.path.join(data, 'PLANS')
    plan_graph.ENUM_FILE = os.environ.get('HB_ENUMIDS', os.path.join(data, 'ENUMIDS.H'))

    for needed in (plans_dir, plan_graph.ENUM_FILE):
        if not os.path.exists(needed):
            print('missing %s; this reads the game data and does not ship it' % needed)
            return 1

    paths = []

    for wanted in options.plans or sorted(glob.glob(os.path.join(plans_dir, '*.PBN'))):
        path = wanted

        if not os.path.exists(path):
            path = os.path.join(plans_dir, os.path.basename(wanted).upper().replace('.PBN', '') + '.PBN')

        if not os.path.exists(path):
            print('missing %s' % path)
            return 1

        paths.append(path)

    dumper = Dumper(plan_graph.enums(), plans_dir, sys.stdout, options.kinds)

    if options.decisions:
        for path in paths:
            Decisions(dumper, sys.stdout).report(path)

        return 0

    for path in paths:
        try:
            dumper.dump(path)
        except (IndexError, struct.error):
            print('%s does not parse' % path)
            return 1

        sys.stdout.write('\n')

    return 0


if __name__ == '__main__':
    sys.exit(main())
