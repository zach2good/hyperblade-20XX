"""Decode a compiled plan file into named behaviours, messages and actions.

A `.PBN` under `DATA/PLANS` is the game's own state machine for one kind of object.

    python tools/plan_graph.py INPUT            # the input device's plan, named
    python tools/plan_graph.py HELMCAM --raw    # numbers rather than names
    python tools/plan_graph.py --sends 78       # every plan that sends id_MSG_HeadCamOn
    python tools/plan_graph.py --list           # the plan files and their behaviours

Format: a file opens with a dword count and is `(count + 1) * 4` bytes. The stream is compound
nodes behind the sentinel `ff ef cd ab`: an opcode dword `(category << 16) | sub`, a child count
dword, then the children. Each child is another compound, a string behind `fe ef cd ab` with one
character per dword, or a raw leaf. `END_PLAN` is the one compound with no count dword.

Names: every id in the stream indexes an enum in `DATA/ENUMIDS.H`, the developers' own header.
It is game data, read at run time and never copied into this repository. Messages come from
`ID_MSG`, actions from `ActionFunctionIDs`, the arms of a `DECIDE_BY_AMONG` from
`DecisionFunctionIDs`, agenda items from `AgendaItemIDs` and a `DECLARE_BEHAVIOR`'s class from
`Behavior_IDs`. Without that file the numbers still print.

Exits 0 when the file parses, 1 when it does not.
"""

import glob
import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PLANS = os.path.join(ROOT, 'build', 'out', 'Data', 'PLANS')
ENUM_FILE = os.environ.get('HB_ENUMIDS', os.path.join(ROOT, 'build', 'out', 'Data', 'ENUMIDS.H'))

COMPOUND = 0xABCDEFFF
STRING = 0xABCDEFFE
END_PLAN = 0xFFFF0000

OPCODES = {
    0x00030000: 'SET_COLLISION_INTERACTOR',
    0x00030001: 'SET_LOC_COLLISION_INTERACTOR',
    0x00050000: 'SET_MESSAGE_INTERACTOR',
    0x00050001: 'SET_LOC_MESSAGE_INTERACTOR',
    0x00050002: 'SET_NET_MESSAGE_INTERACTOR',
    0x00060001: 'SET_DEBUG',
    0x00060002: 'SET_TRACE',
    0x00070000: 'DECLARE_BEHAVIOR',
    0x00070002: 'CHANGE_TO_BEHAVIOR',
    0x00070003: 'SET_BEHAVIOR_PARAM',
    0x00160000: 'CHANGE_OF_PLAN',
    0x00160001: 'DECLARE_LOCAL_REALS',
    0x00170000: 'DECIDE_BY_AMONG',
    0x00170001: 'DECIDE_BY_WITH_AMONG',
    0x00180000: 'DO_ACTION',
    0x00180001: 'SET_INTERACTOR_PARAM',
    0x00180002: 'DO_LOCAL_ACTION',
    0x00180003: 'DO_NET_ACTION',
    0x001A0001: 'ADD_AGENDA_ITEM',
    0x001A0002: 'SET_TIMEOUTMSG',
    0x001A0003: 'REMOVE_AGENDA_ITEM',
    0x001B0000: 'GLOBAL_INTERACTORS',
    0x80000000: 'Enable',
    0x40000000: 'Block',
    0x20000000: 'DataBlock',
}

# The plan compiler's own token table, seeded in PlanVM's constructor. _BROADCAST_ is 0xffff,
# which is too common a number to name.
SENTINELS = {
    0xABCDABCD: '_STRAIGHTAHEAD_',
    0xABCDABCC: '_TEMPORARILY_',
    0xABCDABCE: '_MESSAGE_DATA_',
    0xABCDABCB: '_NO_BEH_CHANGE_',
    0xABCDABCA: '_PREVIOUS_BEH_',
}

ACTIONS = (0x00180000, 0x00180002, 0x00180003)
DECLARE = 0x00070000
CHANGE = 0x00070002

# Which enum names the leaf at each position of each opcode. A message interactor's first child
# is a message, an action's first child is an action function, and so on.
ARGUMENT_ENUM = {
    0x00050000: {0: 'ID_MSG'},
    0x00050001: {0: 'ID_MSG'},
    0x00050002: {0: 'ID_MSG'},
    0x00170000: {0: 'DecisionFunctionIDs'},
    0x00170001: {0: 'DecisionFunctionIDs'},
    0x00180000: {0: 'ActionFunctionIDs'},
    0x00180002: {0: 'ActionFunctionIDs'},
    0x00180003: {0: 'ActionFunctionIDs'},
    0x001A0001: {0: 'AgendaItemIDs'},
    0x001A0002: {1: 'ID_MSG'},
    0x001A0003: {0: 'AgendaItemIDs'},
    DECLARE: {0: 'Behavior_IDs'},
}

# The two actions that carry a message id, and where it sits among their arguments. Both take
# the recipient first, so the message is the one after it.
MESSAGE_ACTIONS = {'id_ACF_SendMessage': 2, 'id_ACF_ReallySendMsg': 2}


def enums():
    """Every enum in the developers' own header, by name, read at run time and never copied."""
    if not os.path.exists(ENUM_FILE):
        return {}

    text = open(ENUM_FILE, encoding='latin-1').read()
    text = re.sub(r'//.*', '', text)
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)

    found = {}

    for match in re.finditer(r'enum\s+(\w+)\s*\{(.*?)\}', text, re.S):
        found[match.group(1)] = re.findall(r'\b(id_\w+)\b', match.group(2))

    return found


def read(path):
    data = open(path, 'rb').read()
    count = struct.unpack_from('<I', data, 0)[0]

    if len(data) != (count + 1) * 4:
        raise SystemExit('%s is %d bytes, not the %d its count asks for'
                         % (path, len(data), (count + 1) * 4))

    return list(struct.unpack_from('<%dI' % count, data, 4))


def parse(words, at=None):
    if at is None:
        at = [0]

    def one():
        i = at[0]
        value = words[i]

        if value == COMPOUND:
            opcode = words[i + 1]

            # END_PLAN carries no child count, so a parser that always reads three dwords runs
            # off the end of the file on the very last node.
            if opcode == END_PLAN:
                at[0] = i + 2
                return ('end', opcode, [])

            children = words[i + 2] & 0xffff
            at[0] = i + 3
            return ('node', opcode, [one() for _ in range(children)])

        if value == STRING:
            j = i + 1
            text = ''

            while words[j] != 0:
                text = text + chr(words[j] & 0xff)
                j = j + 1

            at[0] = j + 1
            return ('str', text, [])

        at[0] = i + 1
        return ('leaf', value, [])

    tops = []

    while at[0] < len(words):
        node = one()
        tops.append(node)

        if node[0] == 'end':
            break

    return tops


def behaviours(tops):
    """The declared behaviours in order, which is the order CHANGE_TO_BEHAVIOR indexes."""
    found = []

    for node in tops:
        if node[0] == 'node' and node[1] == DECLARE:
            name = ''

            for child in node[2]:
                if child[0] == 'str':
                    name = child[1]
                    break

            found.append(name)

    return found


def action_of(tables, children):
    """The name of the action a DO_ACTION node calls, or the empty string."""
    rows = tables.get('ActionFunctionIDs') or []

    if not children or children[0][0] != 'leaf':
        return ''

    first = children[0][1]

    if first >= len(rows):
        return ''

    return rows[first]


class Namer:
    def __init__(self, tables, names, raw):
        self.tables = tables
        self.names = names
        self.raw = raw

    def enum(self, table, value):
        rows = self.tables.get(table)

        if self.raw or not rows or value >= len(rows) or value < 0:
            return '%d' % value

        return '%d %s' % (value, rows[value])

    def behaviour(self, value):
        # A sentinel is not an index: _PREVIOUS_BEH_ is 0xabcdabca and means "go back".
        if value in SENTINELS:
            return SENTINELS[value]

        if self.raw or value >= len(self.names) or value < 0:
            return '%d' % value

        return '%d "%s"' % (value, self.names[value])

    def leaf(self, value):
        if value in SENTINELS:
            return SENTINELS[value]

        if value >> 16 == 0xFEDC:
            return 'ARG[%d]' % (value & 0xffff)

        return '%d' % value


def render(node, namer, depth, out, parent=0, position=0, action=''):
    kind, value, children = node

    if kind == 'end':
        out.write('  ' * depth + 'END_PLAN\n')
        return

    if kind == 'str':
        out.write('  ' * depth + '"%s"\n' % value)
        return

    if kind == 'leaf':
        text = namer.leaf(value)

        if parent == CHANGE:
            text = namer.behaviour(value)
        elif action in MESSAGE_ACTIONS and position == MESSAGE_ACTIONS[action]:
            text = namer.enum('ID_MSG', value)
        else:
            table = ARGUMENT_ENUM.get(parent, {}).get(position)

            if table:
                text = namer.enum(table, value)

        out.write('  ' * depth + text + '\n')
        return

    # An arm of a DECIDE_BY_AMONG is picked by the decision function's own answer, so the
    # arm NUMBER is the whole content: for id_DCF_MessageData it is the message payload, and
    # in the input dispatch that payload is the action id out of g_aActionBindings.
    arm = ''

    if parent in (0x00170000, 0x00170001) and position > 0:
        arm = 'arm %d: ' % (position - 1)

    out.write('  ' * depth + arm
              + OPCODES.get(value, 'op_%04x_%04x' % (value >> 16, value & 0xffff)) + "\n")

    # An action's own name decides what its later arguments mean, so it is carried down.
    mine = ''

    if value in ACTIONS:
        mine = action_of(namer.tables, children)

    for index in range(len(children)):
        render(children[index], namer, depth + 1, out, value, index, mine)


def walk_sends(tables, node, owner, message, hits):
    kind, value, children = node

    if kind == 'node' and value == DECLARE:
        for child in children:
            if child[0] == 'str':
                owner = child[1]
                break

    if kind == 'node' and value in ACTIONS:
        leaves = [c[1] for c in children if c[0] == 'leaf']
        action = action_of(tables, children)
        where = MESSAGE_ACTIONS.get(action)

        if where is not None and len(leaves) > where and leaves[where] == message:
            hits.append((owner, action, leaves))

    for child in children:
        walk_sends(tables, child, owner, message, hits)


def sends(tables, message):
    """Every plan file that sends one message, and the behaviour it sends it from."""
    rows = tables.get('ID_MSG') or []
    wanted = '%d' % message

    if message < len(rows):
        wanted = '%d %s' % (message, rows[message])

    print('who sends %s' % wanted)
    total = 0

    for path in sorted(glob.glob(os.path.join(PLANS, '*.PBN'))):
        hits = []

        for node in parse(read(path)):
            walk_sends(tables, node, '(global)', message, hits)

        for owner, action, leaves in hits:
            total = total + 1
            print('    %-14s %-24s %-22s args %s'
                  % (os.path.basename(path), owner, action, leaves))

    print()
    print('%d send site(s)' % total)
    return 0


def verify():
    """Every plan file has to parse to its last word and come out the right shape at the top.

    The word count alone proves nothing: a parser that under-consumes produces more top-level
    nodes and still lands on the last word. So the shape is checked too: a run of compounds, the
    last one END_PLAN and every one before it GLOBAL_INTERACTORS or DECLARE_BEHAVIOR. Masking the
    child count to a byte cannot be detected, since no shipped node has more than 255 children.
    """
    faults = []
    checked = 0
    allowed = (0x001B0000, DECLARE)

    for path in sorted(glob.glob(os.path.join(PLANS, '*.PBN'))):
        name = os.path.basename(path)
        words = read(path)
        at = [0]
        checked = checked + 1

        try:
            tops = parse(words, at)
        except IndexError:
            faults.append('%s runs off the end of the file' % name)
            continue

        if at[0] != len(words):
            faults.append('%s parses %d of its %d words' % (name, at[0], len(words)))

        if not tops or tops[-1][0] != 'end':
            faults.append('%s does not end on END_PLAN' % name)

        for node in tops[:-1]:
            if node[0] != 'node':
                faults.append('%s has a stray %s at the top level' % (name, node[0]))
            elif node[1] not in allowed:
                faults.append('%s has %s at the top level'
                              % (name, OPCODES.get(node[1], '%08x' % node[1])))

    if faults:
        print('FAULT: %d thing(s) wrong across %d plan file(s):' % (len(faults), checked))

        for one in faults:
            print('    %s' % one)

        return 1

    print('all %d plan file(s) parse to their last word and are a run of behaviours'
          % checked)
    return 0


def listing():
    print('%-14s %s' % ('plan', 'behaviours, in the order CHANGE_TO_BEHAVIOR indexes'))

    for path in sorted(glob.glob(os.path.join(PLANS, '*.PBN'))):
        names = behaviours(parse(read(path)))
        print('%-14s %s' % (os.path.basename(path), ', '.join(names) or '(none)'))

    return 0


def main():
    if not os.path.isdir(PLANS):
        print('no %s; this reads the game data and does not ship it' % PLANS)
        return 0

    tables = enums()

    if not tables:
        print('DATA/ENUMIDS.H is not to hand, so every id prints as a number')

    if '--verify' in sys.argv:
        return verify()

    if '--list' in sys.argv:
        return listing()

    if '--sends' in sys.argv:
        return sends(tables, int(sys.argv[sys.argv.index('--sends') + 1]))

    wanted = [a for a in sys.argv[1:] if not a.startswith('--')]

    if not wanted:
        print('name a plan, or pass --list or --sends N')
        return 1

    path = os.path.join(PLANS, wanted[0].upper().replace('.PBN', '') + '.PBN')

    if not os.path.exists(path):
        print('no such plan: %s' % path)
        return 1

    tops = parse(read(path))
    namer = Namer(tables, behaviours(tops), '--raw' in sys.argv)

    for node in tops:
        render(node, namer, 0, sys.stdout)

    return 0


if __name__ == '__main__':
    sys.exit(main())
