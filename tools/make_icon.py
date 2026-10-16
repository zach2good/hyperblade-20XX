"""Draw the app icon, the Rok between four darts, and write it as a multi-size .ico.

The shapes are drawn here from scratch on a 256-unit grid; nothing is taken from the game.

    python tools/make_icon.py                  writes res/hyperblade_20XX.ico
    python tools/make_icon.py --preview x.png  also writes the 256-pixel image as a PNG
"""

import argparse
import os
import struct
import zlib

SIZES = [16, 24, 32, 48, 64, 128, 256]
SUPERSAMPLE = 4

# Back to front: four darts pointing in at the Rok, then its four faces, lit from the upper left,
# in the colours it takes in a match. Every shape mirrors across both centre lines.
LIGHT_DART = (255, 232, 24)
DARK_DART = (206, 172, 0)

SHAPES = [
    ((114, 8), (128, 8), (128, 56), LIGHT_DART),
    ((128, 8), (142, 8), (128, 56), DARK_DART),
    ((114, 248), (128, 248), (128, 200), LIGHT_DART),
    ((128, 248), (142, 248), (128, 200), DARK_DART),
    ((8, 114), (8, 128), (56, 128), LIGHT_DART),
    ((8, 128), (8, 142), (56, 128), DARK_DART),
    ((248, 114), (248, 128), (200, 128), LIGHT_DART),
    ((248, 128), (248, 142), (200, 128), DARK_DART),
    ((84, 128), (128, 84), (128, 128), (255, 214, 72)),
    ((128, 84), (172, 128), (128, 128), (255, 150, 32)),
    ((84, 128), (128, 128), (128, 172), (232, 84, 24)),
    ((128, 128), (172, 128), (128, 172), (176, 40, 20)),
]


def inside_triangle(x, y, a, b, c):
    def side(p, q):
        return (q[0] - p[0]) * (y - p[1]) - (q[1] - p[1]) * (x - p[0])

    d1 = side(a, b)
    d2 = side(b, c)
    d3 = side(c, a)
    has_negative = d1 < 0 or d2 < 0 or d3 < 0
    has_positive = d1 > 0 or d2 > 0 or d3 > 0
    return not (has_negative and has_positive)


def sample(x, y):
    colour = None

    for a, b, c, fill in SHAPES:
        if inside_triangle(x, y, a, b, c):
            colour = fill

    return colour


def render(size):
    pixels = bytearray()
    step = 256 / (size * SUPERSAMPLE)

    for row in range(size):
        for column in range(size):
            red = green = blue = covered = 0
            for sy in range(SUPERSAMPLE):
                for sx in range(SUPERSAMPLE):
                    x = (column * SUPERSAMPLE + sx + 0.5) * step
                    y = (row * SUPERSAMPLE + sy + 0.5) * step
                    colour = sample(x, y)
                    if colour is not None:
                        red += colour[0]
                        green += colour[1]
                        blue += colour[2]
                        covered += 1

            count = SUPERSAMPLE * SUPERSAMPLE
            if covered == 0:
                pixels += bytes((0, 0, 0, 0))
            else:
                pixels += bytes((red // covered, green // covered, blue // covered, 255 * covered // count))

    return bytes(pixels)


def png(size, rgba):
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

    rows = b"".join(b"\0" + rgba[row * size * 4:(row + 1) * size * 4] for row in range(size))
    header = struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(rows, 9)) + chunk(b"IEND", b"")


def ico(images):
    """Every size stored as PNG, which Windows Vista and later read."""
    header = struct.pack("<HHH", 0, 1, len(images))
    offset = len(header) + 16 * len(images)
    directory = b""
    data = b""
    for size, blob in images:
        dimension = size % 256
        directory += struct.pack("<BBBBHHII", dimension, dimension, 0, 0, 1, 32, len(blob), offset + len(data))
        data += blob
    return header + directory + data


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", default=os.path.join(root, "res", "hyperblade_20XX.ico"))
    parser.add_argument("--preview")
    args = parser.parse_args()

    images = [(size, png(size, render(size))) for size in SIZES]

    os.makedirs(os.path.dirname(args.output), exist_ok=True)
    with open(args.output, "wb") as file:
        file.write(ico(images))

    if args.preview:
        with open(args.preview, "wb") as file:
            file.write(images[-1][1])

    print(f"{args.output}: {', '.join(str(size) for size in SIZES)}")


if __name__ == "__main__":
    main()
