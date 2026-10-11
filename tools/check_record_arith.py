"""Fail on byte arithmetic done through a pointer to a record.

When a `char*` local is retyped to the struct it holds, an offset added to it stops being bytes:
with `PhysicsBody* pBody`, `pBody + 0x20` steps 0x20 whole bodies. The compiler accepts it.

This reports `NAME + <number>`, and `NAME + x * <width>`, where NAME is a local, parameter or
member declared as a pointer to a struct or class in src/, or a call to a method that returns
one. A `(char*)` cast in the same parenthesis group makes the step bytes and is not reported.
Arithmetic on a pointer to char, to a scalar, or to a type from outside src/ is not reported.

An index in brackets is not reported, because `pRecords[i]` is legal. A byte offset spelled as
an index, `pEngine[ENGINE_START_FLAG]`, usually fails to compile instead.

    python tools/check_record_arith.py

Exit code 1 when one is found.
"""
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")
LINE = chr(10)

RECORD = re.compile(r"^(?:class|struct) ([0-9A-Za-z_]+)", re.M)

# `PhysicsBody* pBody;`, `PhysicsBody* const pBody = ...`, and the parameter spelling.
DECLARE = r"(?:^[ 	]*|[(,]\s*)(?:const |volatile |struct |class )*%s\s*\*\s*(?:const\s+)?([0-9A-Za-z_]+)\s*(?:[;,)=]|$)"

# The name, a plus or minus, and a literal or macro. A closing parenthesis before the name
# means a cast does the stepping, `(char*)pBody + 0x50`, which is bytes.
ARITH = r"(?<![0-9A-Za-z_>)])%s\s*[-+]\s*(0[xX][0-9a-fA-F]+|[0-9]+|[A-Z][0-9A-Z_]*)(?![0-9A-Za-z_])"

# The strided form, `pEngine + m_nPendingCount * 4 + 0x122c`, whose offset is not one token.
# Anything multiplied by a width and added to a record pointer is meant as bytes.
STRIDED = r"(?<![0-9A-Za-z_>)])%s\s*\+\s*[0-9A-Za-z_]+\s*\*\s*[0-9]+"


def sourceNames(root):
    # Sources live in subdirectories, so walk rather than list. Names are relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def records():
    found = set()

    for name in sorted(sourceNames(SRC)):
        if not name.endswith((".cpp", ".h")):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8", errors="replace").read()

        for hit in RECORD.finditer(text.replace(chr(13) + LINE, LINE)):
            found.add(hit.group(1))

    return found


HEAD = re.compile(r"^[\w:*&<>\s]+?\b[0-9A-Za-z_]+(?:::[~0-9A-Za-z_]+)?\s*\([^;{]*\)\s*"
                  + r"(?:const\s*)?" + LINE + r"?\{", re.M)


def scopes(text):
    """(start, end) of every function body, and the head above it, top level only."""
    found = []

    for match in HEAD.finditer(text):
        at = text.index("{", match.end() - 1)
        depth = 0
        index = at

        while index < len(text):
            if text[index] == "{":
                depth += 1
            elif text[index] == "}":
                depth -= 1

                if not depth:
                    break

            index += 1

        if found and at < found[-1][1]:
            continue

        found.append((match.start(), index))

    return found



# A method that returns a record pointer: `pFoo->GetPhysicsBody() + 0x44` is the same fault.
RETURNS = re.compile(r"^\s+(?:virtual\s+)?([A-Za-z_]\w*)\s*\*\s*([A-Za-z_]\w*)\s*\([^;{]*\)\s*"
                     r"(?:const\s*)?(?:=\s*0\s*)?;", re.M)

CALL = re.compile(r"(?<![0-9A-Za-z_])([A-Za-z_]\w*)\s*\(\s*\)\s*\+\s*([0-9A-Za-z_]+)")


def governedByChar(text, at):
    """True when a (char*) cast applies to the expression this addition sits in.

    The cast has to be in the same parenthesis group, `(char*)x->GetPhysicsBody() + 0x44`. A cast
    elsewhere in the statement applies to something else.
    """
    depth = 0

    for i in range(at - 1, -1, -1):
        ch = text[i]

        if ch == ")":
            depth += 1
        elif ch == "(":
            if depth == 0:
                break

            depth -= 1
        elif ch == ";" and depth == 0:
            break

    # Only the group's own text decides. A cast inside a nested group belongs to that group:
    # `((EngineObject*)((char*)m_pOwner + 8))->GetPhysicsBody() + K` casts the owner, not the
    # body. A nested group that is itself `(char*)` counts as the group's own.
    own = []
    depth = 0
    opened = 0

    for j in range(i + 1, at):
        ch = text[j]

        if ch == "(":
            depth += 1

            if depth == 1:
                opened = j

            continue

        if ch == ")":
            depth -= 1

            if depth == 0:
                nested = text[opened:j + 1]

                if re.match(r"^\(\s*char\s*\*\s*\)$", nested):
                    own.append(nested)
                else:
                    own.append("()")

            continue

        if depth == 0:
            own.append(ch)

    group = "".join(own)

    return "(char*)" in group or "(char *)" in group


def callSites(known):
    """Every `Method() + <offset>` whose method answers a record pointer, uncast."""
    answers = {}

    for name in sorted(sourceNames(SRC)):
        if not name.endswith(".h"):
            continue

        body = open(os.path.join(SRC, name), encoding="utf-8", errors="replace").read()

        for hit in RETURNS.finditer(body.replace(chr(13) + LINE, LINE)):
            if hit.group(1) in known:
                answers[hit.group(2)] = hit.group(1)

    found = []

    for name in sorted(sourceNames(SRC)):
        if not name.endswith(".cpp"):
            continue

        body = open(os.path.join(SRC, name), encoding="utf-8", errors="replace").read()
        body = body.replace(chr(13) + LINE, LINE)

        for hit in CALL.finditer(body):
            if hit.group(1) not in answers:
                continue

            if governedByChar(body, hit.start()):
                continue

            found.append((name, body.count(LINE, 0, hit.start()) + 1, answers[hit.group(1)],
                          "%s() + %s" % (hit.group(1), hit.group(2))))

    return found

MEMBER_DECL = re.compile(r"^[ \t]*(?:const\s+|volatile\s+|mutable\s+|struct\s+|class\s+)*"
                         r"([A-Za-z_]\w*)\s*\*\s*(m_\w+)\s*;", re.M)


def memberPointers(known):
    """Members that unambiguously point at a record, as class -> [name].

    A member name that two headers declare with different types is dropped rather than guessed.
    """
    seen = {}

    for name in sorted(sourceNames(SRC)):
        if not name.endswith(".h"):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8", errors="replace").read()
        text = code(text)

        for kind, member in MEMBER_DECL.findall(text):
            seen.setdefault(member, set()).add(kind)

    out = {}

    for member, kinds in seen.items():
        if len(kinds) == 1 and list(kinds)[0] in known:
            out.setdefault(list(kinds)[0], []).append(member)

    return out


def main():
    known = sorted(records())
    members = memberPointers(set(known))
    found = callSites(set(known))

    for name in sorted(sourceNames(SRC)):
        if not name.endswith((".cpp", ".h")):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8",
                       errors="replace").read().replace(chr(13) + LINE, LINE)

        # The declaration and the use must be in the same body: one file can declare the same
        # local name as `char*` in one function and as a record pointer in another.
        for start, end in scopes(text):
            body = text[start:end]

            for klass in known:
                if klass not in body:
                    continue

                names = [hit.group(1)
                         for hit in re.finditer(DECLARE % re.escape(klass), body, re.M)]

                for member in members.get(klass, ()):
                    if re.search(r"(?<![\w>\.])%s\b" % re.escape(member), body):
                        names.append(member)

                for local in names:

                    if re.search(r"(?:^[ 	]*|[(,]\s*)(?:const |volatile )*char\s*\*\s*"
                                 + r"(?:const\s+)?%s\s*[;,)=]" % re.escape(local), body, re.M):
                        continue

                    for pattern in (ARITH, STRIDED):
                        for use in re.finditer(pattern % re.escape(local), body):
                            found.append((name, text.count(LINE, 0, start + use.start()) + 1,
                                          klass, use.group(0).strip()))

    seen = set()
    rows = []

    for row in found:
        if row[:2] in seen:
            continue

        seen.add(row[:2])
        rows.append(row)

    for name, line, klass, use in rows:
        subject = use.split()[0]
        print("%s:%d  %s answers a %s*, so `%s` steps whole records"
              % (name, line, subject, klass, use)
              if subject.endswith("()")
              else "%s:%d  %s is a %s*, so `%s` steps whole records"
              % (name, line, subject, klass, use))

    print("%d byte offset(s) added to a record pointer" % len(rows))

    if rows:
        print("A retyped local or member kept its offset. Name the field, or keep the char*.")

        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
