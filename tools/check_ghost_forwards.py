# Fails on a `class X;` forward declaration for an X this tree never defines.
#
#     python tools/check_ghost_forwards.py
#     python tools/check_ghost_forwards.py --list
#
# A retired class often leaves its forward declaration behind, because nothing needs it to
# compile. It misleads a reader, and when it sits between a comment and the class the comment
# describes, the prose seems to describe a class that does not exist.
#
# A system type forward declared to avoid its header (HWND__, _iobuf, IDirectDrawSurface) is not
# a ghost. Each is registered below by name rather than hidden by a pattern.
#
# Read only.

import collections
import glob
import os
import re
import sys

from comments import code

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

FORWARD = re.compile(r"^(?:class|struct)\s+(\w+)\s*;", re.M)
DEFINED = re.compile(r"^(?:class|struct|union)\s+(?:__declspec\s*\([^)]*\)\s*)?(\w+)"
                     r"(?:\s*:\s*[^;{]*)?\s*$", re.M)

# A name the system headers define, which this tree forward declares to avoid including them.
OUTSIDE = {
    "HWND__": "what an HWND points at, from <windows.h>",
    "_iobuf": "what a FILE is, from <stdio.h>",
    "IDirectDrawSurface": "a COM interface, from the DirectDraw headers",
    "IDirectDraw": "a COM interface, from the DirectDraw headers",
    "IDirectDrawClipper": "a COM interface, from the DirectDraw headers",
    "IDirect3D": "a COM interface, from the Direct3D headers",
    "IDirect3DDevice": "a COM interface, from the Direct3D headers",
    "IDirect3DViewport": "a COM interface, from the Direct3D headers",
    "IDirectPlay": "a COM interface, from the DirectPlay headers",
    "_D3DDeviceDesc": "what a D3DDEVICEDESC is, from the Direct3D headers",
}

SKIP = ("jpeg/",)


def sources(suffix):
    for path in sorted(glob.glob(os.path.join(ROOT, "src", "**", suffix), recursive=True)):
        shown = os.path.relpath(path, os.path.join(ROOT, "src")).replace(os.sep, "/")

        if not shown.startswith(SKIP):
            yield path, shown


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    defined = set()

    # Definitions include src/jpeg/, though forwards there are not checked: a forward of a
    # type IJG defines, such as jpeg_error_mgr, is answered.
    for suffix in ("*.h", "*.cpp"):
        for path in glob.glob(os.path.join(ROOT, 'src', '**', suffix), recursive=True):
            text = code(open(path, encoding="utf-8", errors="replace").read())
            defined |= set(DEFINED.findall(text))

    ghosts = collections.defaultdict(list)
    registered = 0

    for suffix in ("*.h", "*.cpp"):
        for path, shown in sources(suffix):
            text = code(open(path, encoding="utf-8", errors="replace").read())

            for name in FORWARD.findall(text):
                if name in defined:
                    continue

                if name in OUTSIDE:
                    registered += 1
                    continue

                ghosts[name].append(shown)

    if show or ghosts:
        for name, where in sorted(ghosts.items()):
            print("%-30s %s" % (name, ", ".join(where)))

    if show:
        for name, why in sorted(OUTSIDE.items()):
            print("%-30s registered: %s" % (name, why))

    print("%d class name(s) forward declared that nothing defines, %d registered as a type from "
          "outside src/" % (len(ghosts), registered))

    return 1 if ghosts else 0


if __name__ == "__main__":
    sys.exit(main())
