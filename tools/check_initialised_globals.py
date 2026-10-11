"""Find globals the image gives a value and this tree leaves at zero.

A global that the client initialises in .data reads as its value in the reference image. If
the tree declares the same global without an initialiser, the built program starts with a zero
where the client starts with a pointer, a length or a flag. Nothing in the byte comparison
notices: the comparison reads code.

Markers are read in headers and in .cpp files. Every marker is checked against the bytes its
declared type covers: a builtin's width, times the bound for an array of builtins, and four
bytes for anything else. A global that is zero in the image is skipped, because zero is what an
uninitialised definition already gives. A marker whose definition cannot be found fails.

    python tools/check_initialised_globals.py
"""

import glob
import json
import re
import sys
import urllib.request

from ghidra_api import BASE  # noqa: E402

MARKER = re.compile(r'//\s*GLOBAL:\s*HYPERX\s+0x([0-9a-fA-F]{8})')
NAME = re.compile(r'([A-Za-z_][A-Za-z0-9_]*)\s*(?:\[[^\]]*\]\s*)*[;={]')

# What may stand in front of the name in a definition: a type, and nothing else. It has to
# be there: `g_nCount = 3;` inside a function is an assignment, not a definition.
DEFINITION_HEAD = re.compile(r'^[\w\s\*&:]+$')

# `GameApp g_gameApp(2);`. A prototype matches too, and never shares a name with a global.
CONSTRUCTED = re.compile(r'^([\w\s\*&:]*[\w\*&]\s+\**)([A-Za-z_]\w*)\s*\(')


# Byte widths, so a short is not read as four bytes with its neighbour's value, and a double is
# not passed over because its low word is zero.
WIDTHS = {'char': 1, 'unsigned char': 1, 'signed char': 1, 'bool': 1,
          'short': 2, 'unsigned short': 2,
          'int': 4, 'unsigned int': 4, 'long': 4, 'unsigned long': 4, 'unsigned': 4, 'float': 4,
          'double': 8, '__int64': 8, 'unsigned __int64': 8}

BOUND = re.compile(r'\[\s*(0x[0-9a-fA-F]+|\d+)\s*\]')


def width(decl):
    text = decl.replace('extern', ' ').replace('const', ' ').replace(';', ' ')
    head = text.split('[')[0]
    bounds = BOUND.findall(text)

    if '*' in head:
        element = 4
    else:
        element = WIDTHS.get(re.sub(r'\w+$', ' ', head.strip()).strip())

    # A record's width is not in its declaration, so only its first word is read.
    if element is None:
        return 4

    if text.count('[') != len(bounds):
        return element

    for bound in bounds:
        element *= int(bound, 0)

    return element


def read(address, count):
    url = BASE + '/read_memory?address=0x%08x&length=%d' % (address, count)
    return bytes(json.load(urllib.request.urlopen(url, timeout=30))['data'])


def declared():
    found = {}

    # A .cpp marker sits on the definition itself or on an extern defined in another file, and
    # defined() finds either.
    paths = glob.glob('src/**/*.h', recursive=True) + glob.glob('src/**/*.cpp', recursive=True)

    for path in sorted(paths):
        lines = open(path, encoding='utf-8', newline='').read().replace('\r\n', '\n')

        previous = None

        for line in lines.split('\n'):
            hit = MARKER.search(line)

            if hit:
                previous = int(hit.group(1), 16)
                continue

            if previous is not None:
                name = NAME.search(line.split('//')[0])

                if name:
                    found[name.group(1)] = (previous, line.strip())

                previous = None

    return found


def defined():
    values = {}

    for path in sorted(glob.glob('src/**/*.cpp', recursive=True)):
        lines = open(path, encoding='utf-8', newline='').read().replace('\r\n', '\n')

        for line in lines.split('\n'):
            # A trailing comment may hold an `=` the definition does not.
            stripped = line.split('//')[0].strip()

            if not stripped or stripped.startswith('//') or stripped.startswith('extern "C"'):
                continue

            # `extern int g_x;` in a .cpp is a declaration, and its file may sort before the one
            # holding the definition, which setdefault would then never see. `extern const float
            # g_x = 1.0f;` is a definition, so only the ones with no initialiser are skipped.
            # EXTERN_C counts too: it is how a library's GUID gets a marker.
            if stripped.lower().startswith('extern') and '=' not in stripped:
                continue

            # An object built by its constructor is initialised, and its arguments may run onto
            # the next line.
            built = CONSTRUCTED.match(stripped)

            if built and 'return' not in built.group(1).split():
                values.setdefault(built.group(2), True)
                continue

            # A definition ends at the semicolon on a one line one. A table's initialiser may
            # run on for lines, so any definition with an `=` counts as initialised; only the
            # bare declaration does not.
            opens_table = '=' in stripped

            if (stripped.endswith(';') or opens_table) and (
                    '(' not in stripped.split('=')[0]):
                name = NAME.search(stripped if stripped.endswith(';') else stripped + ';')

                # Only a definition counts: `return g_flCollisionOne;` also matches the name.
                if name is None:
                    continue

                head = stripped[:name.start()]

                # `&& g_bDragOff == 0)` has a head too, but no type in it.
                if DEFINITION_HEAD.match(head) and re.search(r'[A-Za-z_]', head) and (
                        'return' not in head.split()):
                    values.setdefault(name.group(1), '=' in stripped)

    return values


def main():
    names = declared()
    bodies = defined()
    faults = []
    unread = []
    lost = []

    for name in sorted(names):
        # EXTERN_C names a library's GUID, which the library defines.
        if name not in bodies and not names[name][1].startswith('EXTERN_C'):
            lost.append(name)

        if bodies.get(name, None) is not False:
            continue

        # The bridge drops a request now and then, so retry, and say so when a read still fails.
        raw = None

        for attempt in range(3):
            try:
                raw = read(names[name][0], width(names[name][1]))
                break
            except Exception:
                pass

        if raw is None:
            unread.append(name)
            continue

        if raw.strip(b'\0'):
            faults.append((names[name][0], name, raw[::-1].hex()))

    for address, name, value in faults:
        print('0x%08x %-40s image 0x%s, tree leaves it zero' % (address, name, value))

    if unread:
        print('%d could not be read: %s' % (len(unread), ', '.join(sorted(unread))))

    if lost:
        print('%d have no definition this tool can find: %s' % (len(lost), ', '.join(lost)))

    print('%d globals checked, %d start life wrong' % (len(names), len(faults)))

    return 1 if faults or unread or lost else 0


sys.exit(main())
