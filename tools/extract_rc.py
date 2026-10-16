"""Write the original's resources out as a readable .rc script, with its icons as .ico files.

Covers the menu, the dialogs, the string tables, the version block, the icon groups and the
combo box lists in type 240 (RT_DLGINIT). The build itself links the raw leaves through
tools/extract_resources.py; this is for reading them. Nothing written here is committed.

    python tools/extract_rc.py                        # into build/extracted/resources
    python tools/extract_rc.py <original.exe> <output>
"""

import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from extract_resources import resource_leaves
from reference import reference_path

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

RT_ICON = 3
RT_MENU = 4
RT_DIALOG = 5
RT_STRING = 6
RT_GROUP_ICON = 14
RT_VERSION = 16
RT_DLGINIT = 240

MF_POPUP = 0x10
MF_END = 0x80
DS_SETFONT = 0x40
WS_VISIBLE = 0x10000000

CONTROL_CLASSES = {0x80: "Button", 0x81: "Edit", 0x82: "Static", 0x83: "ListBox",
                   0x84: "ScrollBar", 0x85: "ComboBox"}


def quote(text):
    escaped = (text.replace("\\", "\\\\").replace('"', '""')
               .replace("\n", "\\n").replace("\r", "\\r").replace("\t", "\\t"))
    # The script is written as cp1252, so a character it holds goes in as itself; RC.EXE turns
    # a \x escape in a Unicode string into U+FFFD.
    return '"%s"' % "".join(c if ord(c) >= 32 and c.encode("cp1252", "ignore") else
                            "\\x%02x" % ord(c) for c in escaped)


class Reader:
    def __init__(self, data):
        self.data = data
        self.at = 0

    def u16(self):
        value, = struct.unpack_from("<H", self.data, self.at)
        self.at += 2
        return value

    def u32(self):
        value, = struct.unpack_from("<I", self.data, self.at)
        self.at += 4
        return value

    def s16(self):
        value, = struct.unpack_from("<h", self.data, self.at)
        self.at += 2
        return value

    def text(self):
        end = self.at
        while self.data[end:end + 2] != b"\0\0":
            end += 2
        value = self.data[self.at:end].decode("utf-16-le")
        self.at = end + 2
        return value

    def text_or_ordinal(self):
        if self.data[self.at:self.at + 2] == b"\xff\xff":
            self.at += 2
            return self.u16()
        return self.text()

    def align(self):
        self.at = (self.at + 3) & ~3


def menu(name, data):
    reader = Reader(data)
    version = reader.u16()
    reader.u16()

    if version != 0:
        return ["// MENU %s is a MENUEX, which this does not decode" % name]

    lines = ["%s MENU" % name, "BEGIN"]
    depth = 1
    ends = []

    while reader.at < len(data) and depth > 0:
        flags = reader.u16()
        item_id = None if flags & MF_POPUP else reader.u16()
        caption = reader.text()
        indent = "    " * depth

        if flags & MF_POPUP:
            lines.append("%sPOPUP %s" % (indent, quote(caption)))
            lines.append("%sBEGIN" % indent)
            ends.append(bool(flags & MF_END))
            depth += 1
            continue

        if item_id == 0 and caption == "":
            lines.append("%sMENUITEM SEPARATOR" % indent)
        else:
            extra = "".join(", " + option for bit, option in
                            ((0x01, "GRAYED"), (0x02, "INACTIVE"), (0x08, "CHECKED"))
                            if flags & bit)
            lines.append("%sMENUITEM %s, %d%s" % (indent, quote(caption), item_id, extra))

        # An item flagged END closes its popup, and each popup that was itself last closes too.
        if flags & MF_END:
            while True:
                depth -= 1
                lines.append("%sEND" % ("    " * depth))
                if depth == 0 or not ends.pop():
                    break

    return lines


def dialog(name, data):
    reader = Reader(data)
    extended = data[2:4] == b"\xff\xff"

    if extended:
        reader.at = 4
        reader.u32()
        ex_style = reader.u32()
        style = reader.u32()
    else:
        style = reader.u32()
        ex_style = reader.u32()

    count = reader.u16()
    x, y, cx, cy = reader.s16(), reader.s16(), reader.s16(), reader.s16()
    menu_name = reader.text_or_ordinal()
    class_name = reader.text_or_ordinal()
    caption = reader.text()

    lines = ["%s %s %d, %d, %d, %d" % (name, "DIALOGEX" if extended else "DIALOG", x, y, cx, cy),
             "STYLE 0x%08x" % style]

    if ex_style:
        lines.append("EXSTYLE 0x%08x" % ex_style)

    if caption:
        lines.append("CAPTION %s" % quote(caption))

    if menu_name:
        lines.append("MENU %s" % menu_name)

    if class_name:
        lines.append("CLASS %s" % quote(str(class_name)))

    if style & DS_SETFONT:
        points = reader.u16()
        if extended:
            weight = reader.u16()
            italic = reader.data[reader.at]
            charset = reader.data[reader.at + 1]
            reader.at += 2
            lines.append("FONT %d, %s, %d, %d, %d" % (points, quote(reader.text()), weight,
                                                      italic, charset))
        else:
            lines.append("FONT %d, %s" % (points, quote(reader.text())))

    lines.append("BEGIN")

    for _ in range(count):
        reader.align()

        if extended:
            reader.u32()
            item_ex_style = reader.u32()
            item_style = reader.u32()
        else:
            item_style = reader.u32()
            item_ex_style = reader.u32()

        ix, iy, icx, icy = reader.s16(), reader.s16(), reader.s16(), reader.s16()
        item_id = reader.u32() if extended else reader.u16()
        item_class = reader.text_or_ordinal()
        title = reader.text_or_ordinal()
        extra = reader.u16()
        reader.at += extra

        item_class = CONTROL_CLASSES.get(item_class, item_class)
        title = quote(title) if isinstance(title, str) else str(title)
        # CONTROL always adds WS_VISIBLE, so a hidden control has to take it away again.
        style_text = "0x%08x" % item_style
        if not item_style & WS_VISIBLE:
            style_text += " | NOT 0x%08x" % WS_VISIBLE

        line = "    CONTROL %s, %d, %s, %s, %d, %d, %d, %d" % (
            title, item_id, quote(str(item_class)), style_text, ix, iy, icx, icy)

        if item_ex_style:
            line += ", 0x%08x" % item_ex_style

        lines.append(line)

    lines.append("END")
    return lines


def string_table(block, data):
    reader = Reader(data)
    lines = []

    for index in range(16):
        length = reader.u16()
        text = data[reader.at:reader.at + length * 2].decode("utf-16-le")
        reader.at += length * 2

        if length:
            lines.append("    %d, %s" % ((block - 1) * 16 + index, quote(text)))

    return lines


def version_block(data):
    """VS_VERSIONINFO as nested BLOCK and VALUE statements, with the fixed part first."""
    lines = []
    fixed_at = data.find(b"\xbd\x04\xef\xfe")

    if fixed_at >= 0:
        fields = struct.unpack_from("<13I", data, fixed_at)
        file_version = (fields[2] >> 16, fields[2] & 0xFFFF, fields[3] >> 16, fields[3] & 0xFFFF)
        product = (fields[4] >> 16, fields[4] & 0xFFFF, fields[5] >> 16, fields[5] & 0xFFFF)
        lines += ["1 VERSIONINFO",
                  "FILEVERSION %d,%d,%d,%d" % file_version,
                  "PRODUCTVERSION %d,%d,%d,%d" % product,
                  "FILEFLAGSMASK 0x%x" % fields[6],
                  "FILEFLAGS 0x%x" % fields[7],
                  "FILEOS 0x%x" % fields[8],
                  "FILETYPE 0x%x" % fields[9],
                  "FILESUBTYPE 0x%x" % fields[10],
                  "BEGIN"]

    def walk(at, end, depth):
        while at < end:
            length, value_length, kind = struct.unpack_from("<HHH", data, at)
            if length == 0:
                break

            reader = Reader(data)
            reader.at = at + 6
            key = reader.text()
            reader.align()
            indent = "    " * depth

            if depth == 0:
                # The root: its value is the fixed part, written above.
                walk((reader.at + value_length + 3) & ~3, at + length, depth + 1)
            elif key in ("StringFileInfo", "VarFileInfo") or (depth == 2 and value_length == 0):
                lines.append("%sBLOCK %s" % (indent, quote(key)))
                lines.append("%sBEGIN" % indent)
                walk(reader.at, at + length, depth + 1)
                lines.append("%sEND" % indent)
            elif kind == 1:
                value = data[reader.at:reader.at + value_length * 2].decode("utf-16-le")
                lines.append("%sVALUE %s, %s" % (indent, quote(key), quote(value.rstrip("\0"))))
            else:
                words = struct.unpack_from("<%dH" % (value_length // 2), data, reader.at)
                lines.append("%sVALUE %s, %s" % (indent, quote(key),
                                                 ", ".join("0x%x" % w for w in words)))

            at = (at + length + 3) & ~3

    walk(0, len(data), 0)
    lines.append("END")
    return lines


def dialog_init(name, data):
    """RT_DLGINIT as a readable listing, then as raw words, since RC has no statement for it."""
    lines = ["// DLGINIT %s: the strings each combo box starts with" % name]
    at = 0

    while at + 2 <= len(data):
        control, = struct.unpack_from("<H", data, at)
        if control == 0:
            break

        message, length = struct.unpack_from("<HI", data, at + 2)
        payload = data[at + 8:at + 8 + length].rstrip(b"\0").decode("cp1252")
        lines.append("//     control %d, message 0x%04x: %s" % (control, message, quote(payload)))
        at += 8 + length

    words = struct.unpack_from("<%dH" % (len(data) // 2), data)
    lines += ["%s %d" % (name, RT_DLGINIT), "BEGIN"]
    lines += ["    " + ", ".join("0x%04x" % w for w in words[i:i + 8]) + ","
              for i in range(0, len(words), 8)]
    lines[-1] = lines[-1].rstrip(",")
    lines.append("END")
    return lines


def icon_file(group, icons):
    """An RT_GROUP_ICON and its RT_ICON leaves as one .ico file."""
    reserved, kind, count = struct.unpack_from("<HHH", group, 0)
    entries = []
    images = []

    for index in range(count):
        width, height, colours, _, planes, bits, size, icon_id = struct.unpack_from(
            "<BBBBHHIH", group, 6 + index * 14)
        entries.append((width, height, colours, planes, bits, size))
        images.append(icons[icon_id])

    offset = 6 + 16 * count
    out = struct.pack("<HHH", reserved, kind, count)

    for (width, height, colours, planes, bits, size), image in zip(entries, images):
        out += struct.pack("<BBBBHHII", width, height, colours, 0, planes, bits, len(image), offset)
        offset += len(image)

    return out + b"".join(images)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("original", nargs="?", help="the original executable (default: your copy)")
    parser.add_argument("output", nargs="?",
                        default=os.path.join(ROOT, "build", "extracted", "resources"))
    args = parser.parse_args()

    leaves = resource_leaves(args.original or reference_path())
    by_type = {}

    for leaf in leaves:
        by_type.setdefault(leaf.type_id, []).append(leaf)

    os.makedirs(args.output, exist_ok=True)
    script = ["// Resources of HYPERX.EXE, written by tools/extract_rc.py.", "",
              "#pragma code_page(1252)", ""]
    icons = {leaf.name_id: leaf.data for leaf in by_type.get(RT_ICON, [])}

    for leaf in sorted(by_type.get(RT_GROUP_ICON, []), key=lambda leaf: str(leaf.name_id)):
        target = os.path.join(args.output, "%s.ico" % leaf.name_id)
        with open(target, "wb") as handle:
            handle.write(icon_file(leaf.data, icons))
        script += ["%s ICON %s" % (leaf.name_id, quote(os.path.basename(target))), ""]

    for leaf in by_type.get(RT_MENU, []):
        script += menu(str(leaf.name_id), leaf.data) + [""]

    for leaf in sorted(by_type.get(RT_DIALOG, []), key=lambda leaf: leaf.name_id):
        script += dialog(str(leaf.name_id), leaf.data) + [""]

    script += ["STRINGTABLE", "BEGIN"]
    for leaf in sorted(by_type.get(RT_STRING, []), key=lambda leaf: leaf.name_id):
        script += string_table(leaf.name_id, leaf.data)
    script += ["END", ""]

    for leaf in by_type.get(RT_VERSION, []):
        script += version_block(leaf.data) + [""]

    for leaf in sorted(by_type.get(RT_DLGINIT, []), key=lambda leaf: leaf.name_id):
        script += dialog_init(str(leaf.name_id), leaf.data) + [""]

    known = {RT_ICON, RT_MENU, RT_DIALOG, RT_STRING, RT_GROUP_ICON, RT_VERSION, RT_DLGINIT}
    for type_id in sorted(set(by_type) - known, key=str):
        for leaf in by_type[type_id]:
            target = os.path.join(args.output, "type%s_%s.bin" % (type_id, leaf.name_id))
            with open(target, "wb") as handle:
                handle.write(leaf.data)
            script.append("// type %s, %s: raw, in %s" % (type_id, leaf.name_id,
                                                          os.path.basename(target)))

    target = os.path.join(args.output, "HYPERX.rc")
    with open(target, "w", encoding="cp1252", newline="\r\n") as handle:
        handle.write("\n".join(script) + "\n")

    print("%d resource(s) written to %s" % (len(leaves), args.output))

    return 0


if __name__ == "__main__":
    sys.exit(main())
