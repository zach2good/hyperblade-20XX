"""Unpack the game's MDM1-packed bitmaps into ordinary BMP files, and PNG with --png.

MDM1 is the LZSS packing GDI::ReadBitmapFile reads: the tag "MDM1", the unpacked size as a
dword, then the stream MDM1::LZSSDecompress decodes. What it unpacks is a DIB with no file
header, so this adds one. The game data is yours and nothing written here is committed.

    python tools/extract_mdm1.py                      # build/out/Data to build/extracted/mdm1
    python tools/extract_mdm1.py <Data> <output> --png
"""

import argparse
import os
import struct
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MAGIC = b"MDM1"


def unpack(packed):
    """Decode one MDM1 stream; the length check is the same one the game makes."""
    size = struct.unpack_from("<I", packed, 4)[0]
    data = memoryview(packed)
    out = bytearray()
    at = 8

    while at + 2 <= len(data):
        flags = data[at] | (data[at + 1] << 8)
        at += 2

        for bit in range(16):
            if not flags & (1 << bit):
                if at >= len(data):
                    break

                out.append(data[at])
                at += 1
                continue

            word = data[at] | (data[at + 1] << 8)
            at += 2
            start = len(out) - (word >> 4)
            length = (word & 0xF) + 3

            # A length nibble of 15 takes a second length byte. A carry out of it ends the
            # stream, except at 0x10, which is a match of nothing.
            if length == 0x12:
                length += data[at]
                at += 1

                if length > 0xFF:
                    if length & 0xFF == 0x10:
                        continue

                    return bytes(out), size

            for i in range(length):
                out.append(out[start + i])

    return bytes(out), size


def to_bmp(dib):
    header_size, = struct.unpack_from("<I", dib, 0)
    bits, = struct.unpack_from("<H", dib, 14)
    colours, = struct.unpack_from("<I", dib, 32)

    if colours == 0 and bits <= 8:
        colours = 1 << bits

    offset = 14 + header_size + colours * 4
    return b"BM" + struct.pack("<IHHI", 14 + len(dib), 0, 0, offset) + dib


def expand_rle(data, width, height, four):
    """BI_RLE8 or BI_RLE4 pixels as uncompressed 8-bit rows, bottom row first."""
    stride = (width + 3) & ~3
    out = bytearray(stride * height)
    x = 0
    y = 0
    at = 0

    while at + 1 < len(data) and y < height:
        count, value = data[at], data[at + 1]
        at += 2

        if count:
            for i in range(count):
                pixel = (value >> 4 if i % 2 == 0 else value & 0xF) if four else value
                if x < width:
                    out[y * stride + x] = pixel
                x += 1
        elif value == 0:
            x = 0
            y += 1
        elif value == 1:
            break
        elif value == 2:
            x += data[at]
            y += data[at + 1]
            at += 2
        else:
            run = data[at:at + ((value + 1) // 2 if four else value)]
            at += len(run) + (len(run) & 1)
            for i in range(value):
                pixel = (run[i // 2] >> 4 if i % 2 == 0 else run[i // 2] & 0xF) if four else run[i]
                if x < width:
                    out[y * stride + x] = pixel
                x += 1

    return bytes(out)


def to_png(dib):
    """A palette DIB (4 or 8-bit, RLE or not) or a 24-bit one, as PNG; None for anything else."""
    header_size, width, height, _, bits, compression = struct.unpack_from("<IiiHHI", dib, 0)
    colours, = struct.unpack_from("<I", dib, 32)

    if bits not in (4, 8, 24) or compression not in (0, 1, 2):
        return None

    palette = b""
    if bits <= 8:
        colours = colours or 1 << bits
        # JUMSCRN.BMP declares 257 colours; an 8-bit pixel reaches only the first 256.
        quads = dib[header_size:header_size + min(colours, 1 << bits) * 4]
        palette = b"".join(bytes((quads[i + 2], quads[i + 1], quads[i]))
                           for i in range(0, len(quads), 4))
    else:
        colours = 0

    pixels = dib[header_size + colours * 4:]

    if compression:
        pixels = expand_rle(pixels, width, height, compression == 2)
        bits = 8

    row_bytes = (width * bits + 7) // 8
    stride = (row_bytes + 3) & ~3
    rows = []

    for y in range(abs(height)):
        source = abs(height) - 1 - y if height > 0 else y
        row = pixels[source * stride:source * stride + row_bytes]

        if bits == 24:
            row = bytearray(row)
            row[0::3], row[2::3] = row[2::3], row[0::3]

        rows.append(b"\0" + bytes(row))

    def chunk(kind, payload):
        crc = zlib.crc32(kind + payload) & 0xFFFFFFFF
        return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", crc)

    # PNG packs a 4-bit row high nibble first, as a DIB does, so palette rows copy across.
    depth = bits if palette else 8
    colour_type = 3 if palette else 2
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, abs(height), depth,
                                                             colour_type, 0, 0, 0))
    if palette:
        png += chunk(b"PLTE", palette)

    return png + chunk(b"IDAT", zlib.compress(b"".join(rows), 6)) + chunk(b"IEND", b"")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("data", nargs="?", default=os.path.join(ROOT, "build", "out", "Data"),
                        help="the game's Data folder (default build/out/Data)")
    parser.add_argument("output", nargs="?",
                        default=os.path.join(ROOT, "build", "extracted", "mdm1"))
    parser.add_argument("--png", action="store_true", help="also write a PNG beside each BMP")
    args = parser.parse_args()

    if not os.path.isdir(args.data):
        raise SystemExit("no Data folder at %s; copy the disc's DATA there or name it" % args.data)

    written = 0
    failed = []
    depths = {}

    for folder, _, names in os.walk(args.data):
        for name in sorted(names):
            source = os.path.join(folder, name)

            with open(source, "rb") as handle:
                packed = handle.read()

            if packed[:4] != MAGIC:
                continue

            dib, size = unpack(packed)

            if len(dib) != size:
                failed.append("%s: %d bytes, header says %d" % (source, len(dib), size))
                continue

            target = os.path.join(args.output, os.path.relpath(source, args.data))
            os.makedirs(os.path.dirname(target), exist_ok=True)

            with open(target, "wb") as handle:
                handle.write(to_bmp(dib))

            bits, = struct.unpack_from("<H", dib, 14)
            depths[bits] = depths.get(bits, 0) + 1

            if args.png:
                png = to_png(dib)

                if png is not None:
                    with open(os.path.splitext(target)[0] + ".png", "wb") as handle:
                        handle.write(png)

            written += 1

    for line in failed:
        print("FAILED %s" % line)

    print("%d MDM1 file(s) unpacked to %s, by bit depth %s" % (written, args.output, depths))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
