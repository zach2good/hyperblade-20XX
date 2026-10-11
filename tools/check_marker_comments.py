"""Report markers that are not followed by the thing they annotate.

reccmp reads the line after a marker as the thing it annotates. Prose there becomes the symbol's
name, and every reference to it scores as a difference. The allowed comments are:

  - A name with no space, such as a mangled CRT symbol in crtglobals.h.
  - Under a VTABLE marker, a commented class or struct declaration such as `// class type_info`.
  - Under a STRING marker, a commented literal. reccmp finds the string by content.

A FUNCTION marker allows no comment at all: the next line must be the definition, or reccmp
silently drops the row.

A STRING marker must be followed by a line holding a string literal. Otherwise reccmp silently
drops the whole enclosing function from the report.

Exit code 1 when either fault is found.

    python tools/check_marker_comments.py
"""
import glob
import re
import sys

MARKER = re.compile(r"^\s*//\s*(GLOBAL|VTABLE|STRING|FUNCTION):\s*HYPERX\s+0x[0-9a-fA-F]+\s*$")

# What reccmp's class_decl_regex accepts as a commented declaration under a VTABLE marker.
CLASS_DECL = re.compile(r"^(class|struct)\s+\S")

bad = []
orphan = []
for path in sorted(glob.glob("src/**/*.h", recursive=True)) + sorted(glob.glob("src/**/*.cpp", recursive=True)):
    lines = open(path, encoding="utf-8", errors="replace").read().split("\n")
    for i, line in enumerate(lines):
        if not MARKER.match(line):
            continue
        after = (lines[i + 1] if i + 1 < len(lines) else "").strip()

        kind = MARKER.match(line).group(1)

        # SYNTHETIC and LIBRARY markers do take a name comment, so MARKER leaves them out.
        if kind == "FUNCTION" and after.startswith("//"):
            bad.append((path, i + 2, after[:70]))

            continue

        if after.startswith("//") and " " in after[2:].strip():
            declared = kind == "VTABLE" and CLASS_DECL.match(after[2:].strip())
            declared = declared or (kind == "STRING" and '"' in after)

            if not declared:
                bad.append((path, i + 2, after[:70]))

        if kind == "STRING" and '"' not in after:
            orphan.append((path, i + 2, after[:70]))

for path, line, text in bad:
    print("PROSE AFTER MARKER %s:%d  %s" % (path, line, text))

for path, line, text in orphan:
    print("STRING MARKER WITH NO LITERAL UNDER IT %s:%d  %s" % (path, line, text))

print("%d markers followed by prose, %d string markers with no literal"
      % (len(bad), len(orphan)))
sys.exit(1 if bad or orphan else 0)
