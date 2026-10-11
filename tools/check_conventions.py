"""Find declarations whose calling convention disagrees with the original's code.

A stub is not scored, so a wrong convention on one shows up only as a loss in its callers.
The tool reads two facts from the original and compares them with the declaration:

- The RET immediate is the size of the stack argument list. __cdecl and varargs pop nothing.
  Other conventions pop each parameter's size (four bytes, or a by-value aggregate's own size),
  less the parameters a __fastcall passes in registers, plus four for an aggregate return's
  hidden pointer. A member's list does not name `this`.
- A read of ECX or EDX before any write says that register carries an argument. The scan stops
  at the first write, call or branch. `XOR reg,reg` and CDQ count as writes.

A __fastcall parameter goes in a register when its type fits in one; a float, double or
by-value aggregate goes on the stack and the next one that fits takes the register. A
__fastcall member spends ECX on `this`. CALLBACK, WINAPI, APIENTRY and PASCAL are __stdcall. Aggregate sizes come from scanning each struct's definition;
a type the scan cannot size counts as four bytes.

Each finding names the check that fired. A wrong RET usually means a miscounted parameter
list; a wrong register usually means the function should be __fastcall or a member.

    python tools/check_conventions.py           # everything with a marker
    python tools/check_conventions.py --stubs   # only the stubs
    python tools/check_conventions.py --sizes   # also print the sized aggregates

Exits 1 on any disagreement. Requires the Ghidra bridge that tools/ghidra_api.py names.
"""
import concurrent.futures
import glob
import re
import sys
import urllib.parse
import urllib.request

from ghidra_api import BASE as BRIDGE  # noqa: E402

MARKER = re.compile(r"^//\s*(STUB|FUNCTION):\s*HYPERX\s+(0x[0-9a-fA-F]+)\s*$")

CONVENTIONS = ("__cdecl", "__stdcall", "__fastcall", "__thiscall")
NAMESPACES = set()
for _h in glob.glob("src/**/*.h", recursive=True) + glob.glob("src/**/*.cpp", recursive=True):
    NAMESPACES.update(re.findall(r"^namespace (\w+)", open(_h, "rb").read().decode("latin1"), re.M))

# The type names a one-token head can be when the head is a return type rather than a
# constructor. See declaredConvention.
BUILTIN_TYPES = ("void", "char", "short", "int", "long", "float", "double", "unsigned",
                 "signed", "bool", "wchar_t")

# The Windows spellings of __stdcall. A callback declared CALLBACK is not a __cdecl one.
STDCALL_ALIASES = ("CALLBACK", "WINAPI", "APIENTRY", "PASCAL")

NAMES = {"ECX": ("ECX", "CX", "CL", "CH"), "EDX": ("EDX", "DX", "DL", "DH")}

# CDQ and CWD write the D register without naming it.
WRITES_EDX = ("CDQ", "CWD", "CQO")

# Everything the tree returns or passes that fits in a register. A return type that is none of
# these and is not a pointer is an aggregate, and an aggregate return adds a hidden pointer to
# the front of the stack argument list, which is four more bytes for the callee to pop.
SCALARS = ("void", "char", "short", "int", "long", "unsigned", "signed", "float", "double",
           "bool", "BOOL", "BYTE", "WORD", "DWORD", "UINT", "LONG", "ULONG", "HRESULT",
           "HWND", "HDC", "HANDLE", "HBITMAP", "HPALETTE", "HINSTANCE", "HMODULE", "HRGN",
           "HBRUSH", "HFONT", "HMENU", "HCURSOR", "HICON", "LRESULT", "LPARAM", "WPARAM",
           "ATOM", "size_t", "time_t", "va_list")

# An enum return comes back in EAX like any other scalar.
ENUMS = set()
for _h in glob.glob("src/**/*.h", recursive=True) + glob.glob("src/**/*.cpp", recursive=True):
    ENUMS.update(re.findall(r"^\s*enum\s+(\w+)", open(_h, "rb").read().decode("latin1"), re.M))

# A class taken by value occupies its own size in the argument list. These are pinned; every
# other aggregate is sized by scanning its definition.
AGGREGATE_PARAMS = {"Vec3": 12, "SurfacePoint": 8}

# Size and alignment of each scalar member type. A member of any other type leaves its whole
# struct unsized, and a parameter of that struct counts four bytes.
MEMBER_SIZES = {"char": 1, "signed char": 1, "unsigned char": 1, "bool": 1, "BYTE": 1,
                "short": 2, "unsigned short": 2, "signed short": 2, "WORD": 2,
                "int": 4, "unsigned int": 4, "signed int": 4, "unsigned": 4, "signed": 4,
                "long": 4, "unsigned long": 4, "signed long": 4, "float": 4,
                "DWORD": 4, "UINT": 4, "LONG": 4, "ULONG": 4, "BOOL": 4, "COLORREF": 4,
                "double": 8, "__int64": 8, "unsigned __int64": 8}

DEFINITION = re.compile(r"\b(?:struct|class)\s+([A-Za-z_][A-Za-z0-9_]*)\s*(:[^{;]*)?\{")

ACCESS = re.compile(r"\b(?:public|private|protected)\s*:")

ARRAY = re.compile(r"\[\s*([0-9]+)\s*\]")


def get(path, **params):
    url = "%s/%s?%s" % (BRIDGE, path, urllib.parse.urlencode(params))

    with urllib.request.urlopen(url, timeout=25) as answer:
        return answer.read().decode("utf-8", "replace")


def tokens(text):
    """The identifier-like words in a piece of operand text."""
    word = ""
    found = []

    for character in text:
        if character.isalnum() or character == "_":
            word += character
            continue

        if word != "":
            found.append(word)
            word = ""

    if word != "":
        found.append(word)

    return found


def readSources():
    """Every marker with the declaration head that follows it."""
    found = []

    for path in sorted(glob.glob("src/**/*.cpp", recursive=True) + glob.glob("src/**/*.h", recursive=True)):
        lines = open(path, encoding="utf-8").read().split("\n")

        for index, line in enumerate(lines):
            hit = MARKER.match(line.strip())

            if hit is None:
                continue

            head = ""

            for step in range(6):
                if index + 1 + step >= len(lines):
                    break

                head += " " + lines[index + 1 + step].strip()

                if head.count("(") > 0 and head.count("(") == head.count(")"):
                    break

            found.append((path, index + 2, hit.group(1), int(hit.group(2), 16), head.strip()))

    return found


def declaredConvention(head):
    before = head.split("(")[0]

    for name in CONVENTIONS:
        if name in before:
            return name

    for name in STDCALL_ALIASES:
        if name in tokens(before):
            return "__stdcall"

    # A virtual defined inside its class body has no qualifier, and is still a member.
    if "virtual" in tokens(before):
        return "__thiscall"

    if "::" in before:
        # A family namespace qualifies a free function, whose default is the caller-cleaned
        # convention; a class qualifier means a member.
        if before.split("::")[0].split()[-1] in NAMESPACES:
            return "__cdecl"
        return "__thiscall"

    # A constructor or destructor defined inside its class has one token before the
    # parenthesis, the class name. A bare return type (`int (*__cdecl f(...))[N]`) and a
    # leading `__declspec(...)` also give one token and are excluded.
    head_tokens = tokens(before)

    while head_tokens and head_tokens[0] in ("virtual", "inline", "static"):
        head_tokens = head_tokens[1:]

    if len(head_tokens) == 1 and head_tokens[0] not in BUILTIN_TYPES:
        if not head_tokens[0].startswith("__"):
            return "__thiscall"

    return "__cdecl"


def returnsAggregate(head):
    """Whether the return type is a struct or class passed through a hidden pointer."""
    before = head.split("(")[0].strip()

    # Drop the function's own name, and the class it belongs to.
    words = before.replace("*", " * ").replace("&", " & ").split()

    if len(words) < 2:
        return False

    words = words[:-1]

    if "*" in words or "&" in words:
        return False

    for word in words:
        if word in SCALARS or word in ENUMS:
            return False

    for word in words:
        if word in ("static", "inline", "const", "extern", "virtual", "explicit"):
            continue

        if word in CONVENTIONS or word in STDCALL_ALIASES:
            continue

        return True

    return False


def declaredParams(head):
    """The parameter list, or None when it cannot be trusted."""
    if "(" not in head or ")" not in head:
        return None

    # Only the first balanced group is the parameter list; a trailing `throw()` is not.
    depth = 0
    start = head.index("(")
    end = -1

    for position in range(start, len(head)):
        if head[position] == "(":
            depth += 1
        elif head[position] == ")":
            depth -= 1

            if depth == 0:
                end = position
                break

    if end < 0:
        return None

    inner = head[start + 1:end].strip()

    if inner in ("", "void"):
        return []

    if "..." in inner or "(*" in inner:
        return None

    depth = 0
    parts = [""]

    for character in inner:
        if character in "(<[":
            depth += 1
        elif character in ")>]":
            depth -= 1

        if character == "," and depth == 0:
            parts.append("")
            continue

        parts[-1] += character

    return [part.strip() for part in parts if part.strip() != ""]


def stripComments(text):
    """The source with its comments blanked, so a brace inside one cannot be counted."""
    out = []
    position = 0
    length = len(text)

    while position < length:
        pair = text[position:position + 2]

        if pair == "//":
            while position < length and text[position] != "\n":
                position += 1

            continue

        if pair == "/*":
            position += 2

            while position < length and text[position:position + 2] != "*/":
                if text[position] == "\n":
                    out.append("\n")

                position += 1

            position += 2
            continue

        out.append(text[position])
        position += 1

    return "".join(out)


def memberSize(statement):
    """A data member's size and alignment, or None when the scan cannot work it out."""
    count = 1
    hit = ARRAY.search(statement)

    if hit is not None:
        count = int(hit.group(1))
        statement = statement[:hit.start()] + " " + statement[hit.end():]

    if "[" in statement or "]" in statement:
        return None

    if "*" in statement or "&" in statement:
        return (4 * count, 4)

    words = statement.replace("const", " ").replace("volatile", " ").split()

    if len(words) < 2:
        return None

    # The last word is the member's name, whatever it is called.
    name = " ".join(words[:-1])

    if name not in MEMBER_SIZES:
        return None

    size = MEMBER_SIZES[name]

    return (size * count, size)


def aggregateSize(body):
    """The size of a type from its member list, or None when any member defeats the scan."""
    if "{" in body or "}" in body:
        return None

    offset = 0
    alignment = 1

    for statement in ACCESS.sub(" ", body).split(";"):
        statement = statement.strip()

        if statement == "":
            continue

        # A function pointer is a member and every other parenthesis is a member function,
        # which takes no storage unless it is virtual, and a virtual one stops the scan.
        if "(*" not in statement and "(" in statement:
            if "virtual" in statement.split():
                return None

            continue

        words = statement.split()

        if "typedef" in words or "friend" in words or "using" in words:
            return None

        if "static" in words:
            continue

        if ":" in statement or "," in statement:
            return None

        measured = memberSize(statement)

        if measured is None:
            return None

        size, memberAlignment = measured

        if offset % memberAlignment != 0:
            offset += memberAlignment - offset % memberAlignment

        offset += size
        alignment = max(alignment, memberAlignment)

    if offset == 0:
        return None

    if offset % alignment != 0:
        offset += alignment - offset % alignment

    return offset


def scanAggregates():
    """Every struct or class in the tree whose size its own definition settles.

    A type the scan cannot size, or that two files define differently, is left out, so a
    parameter of it counts four bytes.
    """
    sizes = {}
    conflicts = set()

    for path in sorted(glob.glob("src/**/*.cpp", recursive=True) + glob.glob("src/**/*.h", recursive=True)):
        text = stripComments(open(path, encoding="utf-8", errors="replace").read())

        for hit in DEFINITION.finditer(text):
            name = hit.group(1)

            # A base class carries members the definition does not list.
            if hit.group(2) is not None:
                conflicts.add(name)
                continue

            depth = 0
            end = -1

            for position in range(hit.end() - 1, len(text)):
                if text[position] == "{":
                    depth += 1
                elif text[position] == "}":
                    depth -= 1

                    if depth == 0:
                        end = position
                        break

            if end < 0:
                continue

            size = aggregateSize(text[hit.end():end])

            if size is None:
                conflicts.add(name)
                continue

            if name in sizes and sizes[name] != size:
                conflicts.add(name)
                continue

            sizes[name] = size

    for name in conflicts:
        sizes.pop(name, None)

    return sizes


def aggregateBytes(part, sizes):
    """A by-value aggregate parameter's own size, or None when the parameter is not one."""
    if "*" in part or "&" in part or "[" in part:
        return None

    words = part.replace("const", " ").replace("volatile", " ").split()

    if len(words) > 1:
        words = words[:-1]

    for word in words:
        if word in AGGREGATE_PARAMS:
            return AGGREGATE_PARAMS[word]

    for word in words:
        if word in sizes:
            # An argument slot is always a whole number of words.
            return sizes[word] + (-sizes[word]) % 4

    return None


def paramBytes(part, sizes):
    """A parameter's own size in the argument list."""
    size = aggregateBytes(part, sizes)

    if size is not None:
        return size

    return 4


def stackBytes(convention, params, sizes, isMember=False):
    """How many bytes the callee should pop, or None when it cannot be worked out."""
    if convention == "__cdecl":
        return 0

    for part in params:
        if "double" in part and aggregateBytes(part, sizes) is None:
            return None

    total = 0

    for part in params:
        total += paramBytes(part, sizes)

    # VC6 gives the registers to the first parameters that fit in one and skips those that do
    # not, as Vec3::ScaleTo's pointers either side of its float show. A __fastcall member
    # spends ECX on `this`.
    if convention == "__fastcall":
        taken = 0
        limit = 1 if isMember else 2

        for part in params:
            if taken == limit:
                break

            if "*" not in part and "&" not in part:
                if aggregateBytes(part, sizes) is not None:
                    continue

                if "float" in part or "double" in part:
                    continue

            taken += 1

        total = max(0, total - taken * 4)

    return total


def firstRegisterUse(listing):
    """Which of ECX and EDX the original reads before writing, as a set."""
    read = set()
    written = set()

    for line in listing.split("\n"):
        body = line.split(":", 1)[-1].split(";")[0].strip()

        if body == "":
            continue

        mnemonic = body.split(" ")[0]

        if mnemonic in WRITES_EDX:
            written.add("EDX")
            continue

        if mnemonic.startswith("CALL") or mnemonic.startswith("J") or mnemonic.startswith("RET"):
            break

        operands = body[len(mnemonic):]
        destination = operands.split(",")[0]
        rest = operands[len(destination):]

        # Zeroing a register with itself writes it and reads nothing.
        if mnemonic in ("XOR", "SUB") and destination.strip() == rest.strip(" ,"):
            rest = ""

        # A destination that is a memory operand reads whatever indexes it.
        indexes = ""

        if "[" in destination:
            indexes = destination[destination.index("["):]

        for register in ("ECX", "EDX"):
            if register in written:
                continue

            for name in NAMES[register]:
                if name in tokens(rest) or name in tokens(indexes):
                    read.add(register)

        for register in ("ECX", "EDX"):
            for name in NAMES[register]:
                if destination.strip() == name:
                    written.add(register)

    return read


def retBytes(listing):
    for line in reversed(listing.strip().split("\n")):
        body = line.split(":", 1)[-1].strip()

        if body.startswith("RET"):
            tail = body[3:].split(";")[0].strip()

            if tail == "":
                return 0

            return int(tail, 16)

    return None


def staticMethods():
    """The names every header declares as a static member function.

    A static member defined as `Class::Fn` has no `this`, so its __fastcall keeps both
    registers for parameters. The definition head cannot say so; only the class body can."""
    names = set()

    for path in sorted(glob.glob("src/**/*.h", recursive=True)):
        owner = None

        for line in open(path, encoding="utf-8").read().splitlines():
            head = re.match(r"^(?:class|struct)\s+(\w+)\b[^;]*$", line)

            if head:
                owner = head.group(1)
                continue

            hit = re.match(r"^\s+static\b[^;{(]*?\b(\w+)\s*\(", line)

            if hit and owner:
                names.add((owner, hit.group(1)))

    return names


def qualifiedName(head):
    """(Class, Function) from a definition head `... Class::Function(`, or None."""
    hit = re.search(r"(\w+)::~?(\w+)\s*$", head.split("(")[0])

    if hit:
        return (hit.group(1), hit.group(2))

    return None


def main():
    onlyStubs = "--stubs" in sys.argv

    entries = readSources()
    sizes = scanAggregates()
    statics = staticMethods()

    if onlyStubs:
        entries = [entry for entry in entries if entry[2] == "STUB"]

    print("checking %d declarations against %d sized aggregates"
          % (len(entries), len(sizes)))

    if "--sizes" in sys.argv:
        for name in sorted(sizes):
            print("  %-32s %d" % (name, sizes[name]))

    def work(entry):
        try:
            return (entry, get("disassemble_function", address="0x%08x" % entry[3]))
        except Exception:
            return None

    findings = []

    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
        for answer in pool.map(work, entries):
            if answer is None:
                continue

            (path, line, kind, address, head), listing = answer

            if listing.strip() == "" or "Error" in listing[:40]:
                continue

            convention = declaredConvention(head)
            params = declaredParams(head)

            # A static member has no `this`: it defaults to __cdecl like a free function.
            if convention == "__thiscall" and qualifiedName(head) in statics:
                convention = "__cdecl"

            expected = None

            if params is not None:
                qualified = head.split("(")[0]
                isMember = "::" in qualified and qualifiedName(head) not in statics and qualified.split("::")[0].split()[-1] not in NAMESPACES

                expected = stackBytes(convention, params, sizes, isMember=isMember)

                # A __cdecl caller cleans the hidden pointer along with everything else.
                if expected is not None and convention != "__cdecl" and returnsAggregate(head):
                    expected += 4

            actual = retBytes(listing)

            if expected is not None and actual is not None and expected != actual:
                findings.append((address, path, line, "RET",
                                 "declared %s with %d args pops %d, the client pops %d"
                                 % (convention, len(params), expected, actual)))
                continue

            used = firstRegisterUse(listing)

            isMember = "::" in head.split("(")[0] and head.split("(")[0].split("::")[0].split()[-1] not in NAMESPACES

            takesEcx = convention in ("__fastcall", "__thiscall")
            # A __fastcall member spends ECX on `this`, so its first parameter is in EDX.
            wanted = 1 if isMember else 2
            # A parameter list the scan cannot read cannot rule EDX out.
            takesEdx = (convention == "__fastcall"
                        and (params is None or len(params) >= wanted))

            if "ECX" in used and not takesEcx:
                findings.append((address, path, line, "ECX",
                                 "declared %s but the client reads ECX before writing it"
                                 % convention))
            elif "EDX" in used and not takesEdx:
                findings.append((address, path, line, "EDX",
                                 "declared %s but the client reads EDX before writing it"
                                 % convention))

    findings.sort()

    print("%d disagreements" % len(findings))

    for address, path, line, kind, message in findings:
        print("  0x%08x %-4s %s:%d" % (address, kind, path, line))
        print("        %s" % message)

    if findings:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
