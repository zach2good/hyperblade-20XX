"""Name the offsets a body reaches through a typed pointer that is not `this`.

tools/self_fields.py and tools/field_names.py handle only `this`. This handles a pointer typed
as the class, such as a free function's `EngineObject* pObject`:

    nState = *(short*)((char*)pObject + OBJECT_STATE);

The rules match self_fields.py:

  - The cast type matches the field: the cast goes and the field is named.
  - It does not: the cast stays and the field is named through it, `*(int*)&pObject->m_flSpeed`.
  - `volatile` always stays. It forces a reload where the client reloads.

It skips a pointer not declared as the class in the same body or its signature, a pointer the
body reassigns, and a private field. An offset the class does not declare is reported.

    python tools/param_fields.py --report EngineObject    # count only, write nothing
    python tools/param_fields.py EngineObject PhysicsBody # rewrite src/ in place
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import self_fields

LINE = chr(10)

DEFINE = re.compile(r"^#define ([0-9A-Za-z_]+) (0x[0-9a-fA-F]+)$", re.M)

RECORD = re.compile(r"^(class|struct) ([0-9A-Za-z_]+)(?: *: *public ([0-9A-Za-z_]+))? *"
                    + LINE + r"\{(.*?)" + LINE + r"\};", re.S | re.M)

FIELD = re.compile(r"^\s+(?:[\w:*&<>]+[\s*]+)+?(m_\w+)\s*(?:\[[^\]]*\])?;", re.M)


def sourceNames(root):
    # Every file under root, recursively, as a path relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def reachable(klass):
    """Field names a non-member may touch: the public ones, on the class and its bases.

    Naming a private member from a free function is a compile error (C2248).
    """
    names = set()

    for name in sorted(sourceNames(SRC)):
        if not name.endswith(".h"):
            continue

        text = open(os.path.join(SRC, name), encoding="utf-8", errors="replace").read()
        text = text.replace(chr(13) + LINE, LINE)

        for hit in RECORD.finditer(text):
            kind, found, base, body = hit.groups()

            if found != klass:
                continue

            access = "public" if kind == "struct" else "private"

            for line in body.split(LINE):
                label = re.match(r"^(public|protected|private)\s*:", line.strip())

                if label:
                    access = label.group(1)
                    continue

                if access != "public":
                    continue

                field = FIELD.match(line + ";")

                if field is None:
                    field = FIELD.match(line)

                if field:
                    names.add(field.group(1))

            if base:
                names |= reachable(base)

    return names


def bodies(text):
    """(start, end) of every brace-balanced function body.

    A column 0 brace counts only when a closing parenthesis ends the text above it, so
    `extern "C"` and `struct` blocks are not read as bodies.
    """
    found = []

    for hit in re.finditer(r"^\{$", text, re.M):
        above = text.rfind(")", 0, hit.start())

        if above < 0 or "\n" in text[above:hit.start()].strip():
            continue

        depth = 0

        for i in range(hit.start(), len(text)):
            if text[i] == "{":
                depth += 1
            elif text[i] == "}":
                depth -= 1

                if depth == 0:
                    found.append((hit.start(), i))
                    break

    return found


def pointers(text, span, klass):
    """Names inside this body that are declared `klass*` and never reassigned."""
    start, end = span
    body = text[start:end]

    # The signature sits above the opening brace; take the line run back to a blank line.
    head = text.rfind(LINE + LINE, 0, start)
    signature = text[head if head > 0 else 0:start]

    found = set()
    declared = r"(?<![0-9A-Za-z_])%s\s*\*\s*(?:const\s+)?([A-Za-z_]\w*)" % re.escape(klass)

    for source in (signature, body):
        for hit in re.finditer(declared, source):
            found.add(hit.group(1))

    # A name the body assigns is no longer what the declaration said.
    kept = set()

    for name in found:
        moved = re.search(r"(?<![0-9A-Za-z_])%s\s*=(?!=)" % re.escape(name), body)

        if moved is None:
            kept.add(name)

    return kept


def rewrite(klass, fields, report, missed):
    """Every body in src/ that holds a `klass*`. Answers how many accesses were named."""
    done = 0

    for name in sorted(sourceNames(SRC)):
        if not name.endswith(".cpp"):
            continue

        path = os.path.join(SRC, name)
        text = open(path, encoding="utf-8").read().replace(chr(13) + LINE, LINE)

        if klass not in text:
            continue

        macros = dict(self_fields.headerMacros() or {})
        macros.update(dict((key, int(value, 16)) for key, value in DEFINE.findall(text)))
        changed = False

        for span in reversed(bodies(text)):
            body = text[span[0]:span[1]]

            for local in sorted(pointers(text, span, klass)):
                access = re.compile(
                    r"(\*?)\(\s*((?:const |volatile |unsigned |signed )*)"
                    r"([0-9A-Za-z_]+)\s*(\*+)\s*\)\s*\(\s*\(\s*char\s*\*\s*\)\s*"
                    + re.escape(local)
                    + r"\s*\+\s*([0-9A-Za-z_]+(?:\s*\+\s*[0-9A-Za-z_]+)*)\s*\)")

                named = [0]

                def swap(hit, local=local):
                    offset = self_fields.total(hit.group(5), macros)

                    if offset is None or offset not in fields:
                        missed.append((klass, name, hit.group(0)[:58]))

                        return hit.group(0)

                    field, kind = fields[offset]
                    stars = hit.group(4)
                    qualifier = hit.group(2).replace("volatile ", "")
                    claimed = qualifier + hit.group(3) + "*" * (len(stars) - 1)
                    reach = "%s->%s" % (local, field)

                    named[0] += 1

                    if claimed == kind and "volatile" not in hit.group(2):
                        return reach if hit.group(1) == "*" else "&" + reach

                    return "%s(%s%s%s)&%s" % (hit.group(1), hit.group(2), hit.group(3),
                                              stars, reach)

                body = access.sub(swap, body)

                if named[0]:
                    done += named[0]
                    changed = True

            if changed:
                text = text[:span[0]] + body + text[span[1]:]

        if changed and not report:
            open(path, "w", encoding="utf-8", newline="").write(text)

    return done


def main():
    report = "--report" in sys.argv[1:]
    wanted = [name for name in sys.argv[1:] if not name.startswith("--")]

    if not wanted:
        print(__doc__)

        return 1

    known = self_fields.records()
    done = 0
    missed = []

    for klass in wanted:
        fields = self_fields.layout(klass, known)

        if not fields:
            print("%s declares no fields this can read" % klass)
            continue

        # A non-member may only touch the public ones.
        open_to_all = reachable(klass)
        fields = dict((at, pair) for at, pair in fields.items()
                      if pair[0].split(".")[0].replace("[0]", "") in open_to_all)

        if not fields:
            print("%s declares no PUBLIC fields this can read" % klass)
            continue

        done += rewrite(klass, fields, report, missed)

    print("%d access(es) named, %d this cannot place" % (done, len(missed)))

    for row in missed[:40]:
        print("    %-22s %-28s %s" % row)

    return 0


if __name__ == "__main__":
    sys.exit(main())
