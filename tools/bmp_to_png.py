"""Turn a bring-up snapshot into a PNG, so it can be looked at without another tool.

The snapshots are 24-bit bottom-up BMPs, as GetDIBits returns them. The conversion is plain
Python, with no image library.

    python tools/bmp_to_png.py <in.bmp> [out.png]
"""
import struct
import sys
import zlib


def convert(source, target):
    data = open(source, "rb").read()

    if data[:2] != b"BM":
        raise SystemExit("%s is not a BMP" % source)

    offset = struct.unpack_from("<I", data, 10)[0]
    width = struct.unpack_from("<i", data, 18)[0]
    height = struct.unpack_from("<i", data, 22)[0]
    depth = struct.unpack_from("<H", data, 28)[0]

    if depth != 24:
        raise SystemExit("%s is %d-bit; only 24 is handled" % (source, depth))

    flipped = height > 0
    height = abs(height)
    stride = ((width * 3 + 3) // 4) * 4

    rows = []

    for y in range(height):
        source_y = height - 1 - y if flipped else y
        start = offset + source_y * stride
        row = data[start:start + width * 3]

        # BMP is BGR and PNG is RGB.
        rows.append(b"\0" + bytes(row[i + 2 - (i % 3) * 2 + (0 if i % 3 == 1 else 0)]
                                  for i in range(len(row))))

    raw = b"".join(rows)

    def chunk(kind, payload):
        return (struct.pack(">I", len(payload)) + kind + payload
                + struct.pack(">I", zlib.crc32(kind + payload) & 0xffffffff))

    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(raw, 6))
           + chunk(b"IEND", b""))

    open(target, "wb").write(png)

    return width, height


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)

    out = sys.argv[2] if len(sys.argv) > 2 else sys.argv[1].rsplit(".", 1)[0] + ".png"
    size = convert(sys.argv[1], out)
    print("%s  %dx%d" % (out, size[0], size[1]))
