"""Fail on a member function that casts `this` to its own class, or to a view class.

A cast of `this` to the enclosing class is dead weight or a mistake: when it is a whole
forwarding stub, it is infinite recursion. symbols/self_casts.csv lists the casts a measurement
keeps, because VC6 allocates registers differently through the cast; a listed function whose
casts have gone is reported as stale. A body that says `KEEP.` above the cast is skipped too.

The second rule catches a cast of `this` to a view class that reaches one of the view's slots,
when the enclosing class already declares that slot. A view is recognised as tools/view_classes.py
does: every slot pure, and at least one named `SlotNN`. A cast reaching a plain member of another
class is how this tree calls the same body with this in ECX, and is not reported.

    python tools/check_self_casts.py

Exit code 1 when one is found.
"""
import csv
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "src")

# A member definition: any return type, then Class::Method(...) and an opening brace.
DEFINITION = re.compile(r"^(?:[\w:*&<>\s]+?)\b(\w+)::(\w+)\s*\([^;{]*\)\s*(?:const\s*)?\{", re.M)

# A stand-in for an object nothing has decoded: pure virtuals, at least one of them anonymous.
# The body is matched loosely, because a view often has comments between its slots.
VIEW = re.compile(r"^class (\w+)\n\{\npublic:\n(.*?)^\};", re.S | re.M)
ANONYMOUS = re.compile(r"\bSlot[0-9A-Fa-f]{1,2}\s*\(")
SLOT = re.compile(r"\bvirtual\s+([^;{]+?)\s*(?:=\s*0\s*)?;", re.S)


def accepted():
    """Casts a measurement keeps, read from symbols/self_casts.csv."""
    rows = {}
    path = os.path.join(ROOT, "symbols", "self_casts.csv")

    if not os.path.exists(path):
        return rows

    with open(path, encoding="utf-8") as handle:
        for row in csv.DictReader(line for line in handle if not line.startswith("#")):
            key = (row["file"].replace("\\", "/"), row["function"])
            rows[key] = row.get("reason", "")

    return rows


def sourceNames(root):
    # Walk subdirectories; names come back relative to root.
    found = []

    for directory, unused, names in os.walk(root):
        for name in names:
            whole = os.path.join(directory, name)
            found.append(os.path.relpath(whole, root).replace(os.sep, "/"))

    return found


def sources(suffixes):
    for name in sorted(sourceNames(SRC)):
        if name.endswith(suffixes):
            path = os.path.join(SRC, name)
            text = open(path, encoding="utf-8", errors="replace").read()

            yield name, text.replace("\r\n", "\n")


def viewClasses():
    """{view name: the names of its virtual slots}."""
    found = {}

    for name, text in sources((".cpp", ".h")):
        for match in VIEW.finditer(text):
            body = match.group(2)

            if "virtual" in body and "= 0" in body and ANONYMOUS.search(body):
                slots = set()

                for declaration in SLOT.findall(body):
                    hit = re.search(r"([0-9A-Za-z_]+)\s*\(", declaration)

                    if hit:
                        slots.add(hit.group(1))

                found[match.group(1)] = slots

    return found


# A local holding the object, such as `char* const pThis = (char*)this;`. A cast of that local is
# the same cast of this. `(char*)this + 0x98` is a sub-object's address, not the object.
HOLDER = re.compile(r"([0-9A-Za-z_]+)" + chr(92) + r"s*=" + chr(92)
                    + r"s*" + chr(92) + r"(" + chr(92) + r"s*(?:const" + chr(92)
                    + r"s+)?[0-9A-Za-z_]+" + chr(92) + r"s*" + chr(92) + r"*" + chr(92)
                    + r"s*" + chr(92) + r")" + chr(92) + r"s*this(?![0-9A-Za-z_])"
                    + r"(?!" + chr(92) + r"s*[-+[])")


# A local the body then moves is no longer the object.
MOVED = re.compile(r"([0-9A-Za-z_]+)" + chr(92) + r"s*(?:" + chr(92) + r"+=|-=|"
                   + chr(92) + r"+" + chr(92) + r"+|--)")

def holders(body):
    return (set(HOLDER.findall(body)) - set(MOVED.findall(body))) | set(["this"])


# A laundering cast in front of the name, as in `((SoundChannel*)(char*)this)`, is ignored.
LAUNDER = r"(?:\(\s*(?:char|void|unsigned char)\s*\*\s*\)\s*)?"


def castTo(klass, names):
    return re.compile(r"\(\(\s*%s\s*\*\s*\)\s*%s(?:%s)\s*\)->(\w+)"
                      % (re.escape(klass), LAUNDER,
                         "|".join(re.escape(name) for name in sorted(names))))


def main():
    views = viewClasses()
    found = []
    onViews = []

    for name, text in sources((".cpp",)):
        for match in DEFINITION.finditer(text):
            klass = match.group(1)
            end = text.find("\n}\n", match.end())
            body = text[match.end():end if end > 0 else len(text)]

            names = holders(body)

            for hit in castTo(klass, names).finditer(body):
                # KEEP, the same word tools/check_dead_this_alias.py honours. Promoting a free
                # function to a member turns the casts already in its body into self casts, and
                # removing them can change the code.
                if "KEEP." in body[:hit.start()]:
                    continue

                line = text.count("\n", 0, match.end() + hit.start()) + 1
                found.append((name, line, klass, match.group(2), hit.group(1)))

            for view in sorted(views):
                if view == klass:
                    continue

                for hit in castTo(view, names).finditer(body):
                    # Only a slot. A cast reaching a plain member of another class is how this
                    # tree calls the same body with this in ECX.
                    if hit.group(1) not in views[view]:
                        continue

                    line = text.count("\n", 0, match.end() + hit.start()) + 1
                    onViews.append((name, line, klass, match.group(2), view, hit.group(1)))

    keep = accepted()
    kept = set()
    live = []

    for row in found:
        key = ("src/" + row[0], row[3])

        if key in keep:
            kept.add(key)
        else:
            live.append(row)

    found = live

    for name, line, klass, method, target in found:
        print("%s:%d  %s::%s casts this to %s to reach %s"
              % (name, line, klass, method, klass, target))

    for name, line, klass, method, view, target in onViews:
        print("%s:%d  %s::%s casts this to the view %s to reach %s"
              % (name, line, klass, method, view, target))

    print("%d view class(es); %d cast(s) of this to the enclosing class, %d to a view"
          % (len(views), len(found), len(onViews)))

    if found:
        print("Each is dead or a self-call. Delete the cast.")

    if onViews:
        print("Each names a slot the enclosing class already has. Call it by that name.")

    stale = sorted(set(keep) - kept)

    for path, function in stale:
        print("STALE  symbols/self_casts.csv keeps %s in %s and it has no cast left"
              % (function, path))

    if found or onViews or stale:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
