#!/usr/bin/env python3
"""Draws the Kinetra application icon and writes every file derived from it.

The icon is defined here as a few vector shapes (a rounded square, a "K" built from
capsules, and two nodes), rasterized with supersampling. It is the single source of:

- resources/kinetra.ico: the Windows executable icon, PNG entries from 16 to 256 px;
- resources/kinetra-icon.png: the 256 px image, for documentation and desktop files;
- src/runtime/window_icon.inc: the 64 px RGBA pixels that Runtime gives every window.

Usage:
    python3 tools/make_icon.py          # regenerate the three files
    python3 tools/make_icon.py --check  # exit 1 if a committed file differs

The check compares decoded pixels, not file bytes, because zlib versions may compress
the same pixels differently. Only the standard library is used.
"""

import argparse
import math
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ICO_PATH = ROOT / "resources" / "kinetra.ico"
PNG_PATH = ROOT / "resources" / "kinetra-icon.png"
INC_PATH = ROOT / "src" / "engine" / "window_icon.inc"

ICO_SIZES = (16, 24, 32, 48, 64, 128, 256)
WINDOW_SIZE = 64
# Samples per pixel along each axis; 4x4 keeps edges smooth at 16 px.
SUPERSAMPLING = 4

# Colors as linear 0..1 RGB tuples (sRGB values; blending in sRGB is close enough here).
BACKGROUND_TOP = (0.106, 0.118, 0.294)  # deep indigo
BACKGROUND_BOTTOM = (0.035, 0.400, 0.420)  # teal
STROKE = (0.945, 0.961, 0.976)  # near white
NODE_UPPER = (0.976, 0.600, 0.090)  # amber
NODE_LOWER = (0.180, 0.830, 0.900)  # cyan

# Shapes in unit coordinates, y downwards, matching image rows.
FRAME_INSET = 0.035
FRAME_RADIUS = 0.21
BAR = ((0.31, 0.24), (0.31, 0.76), 0.060)  # (from, to, half width)
UPPER_ARM = ((0.35, 0.53), (0.67, 0.29), 0.052)
LOWER_ARM = ((0.42, 0.47), (0.67, 0.72), 0.052)
UPPER_NODE = ((0.695, 0.275), 0.088)  # (center, radius)
LOWER_NODE = ((0.695, 0.735), 0.088)


def rounded_square(x, y):
    """Signed distance to the rounded frame; negative inside."""
    half = 0.5 - FRAME_INSET
    qx = abs(x - 0.5) - (half - FRAME_RADIUS)
    qy = abs(y - 0.5) - (half - FRAME_RADIUS)
    outside = math.hypot(max(qx, 0.0), max(qy, 0.0))
    return outside + min(max(qx, qy), 0.0) - FRAME_RADIUS


def capsule(x, y, shape):
    """Signed distance to a line segment with round caps."""
    (ax, ay), (bx, by), half_width = shape
    px, py, dx, dy = x - ax, y - ay, bx - ax, by - ay
    t = max(0.0, min(1.0, (px * dx + py * dy) / (dx * dx + dy * dy)))
    return math.hypot(px - dx * t, py - dy * t) - half_width


def disc(x, y, shape):
    (cx, cy), radius = shape
    return math.hypot(x - cx, y - cy) - radius


def sample(x, y):
    """Straight RGBA color of the icon at unit point (x, y)."""
    if rounded_square(x, y) > 0:
        return (0.0, 0.0, 0.0, 0.0)
    t = y
    color = tuple(a + (b - a) * t for a, b in zip(BACKGROUND_TOP, BACKGROUND_BOTTOM))
    # Painter's order: the K first, then the nodes on top of the arm ends.
    if min(capsule(x, y, BAR), capsule(x, y, UPPER_ARM), capsule(x, y, LOWER_ARM)) <= 0:
        color = STROKE
    if disc(x, y, UPPER_NODE) <= 0:
        color = NODE_UPPER
    if disc(x, y, LOWER_NODE) <= 0:
        color = NODE_LOWER
    return color + (1.0,)


def render(size):
    """Returns size*size*4 RGBA bytes, rows top to bottom, straight (not premultiplied) alpha."""
    pixels = bytearray()
    n = SUPERSAMPLING
    for row in range(size):
        for column in range(size):
            r = g = b = a = 0.0
            for sy in range(n):
                for sx in range(n):
                    cr, cg, cb, ca = sample((column + (sx + 0.5) / n) / size, (row + (sy + 0.5) / n) / size)
                    r += cr * ca
                    g += cg * ca
                    b += cb * ca
                    a += ca
            if a == 0:
                pixels += b"\0\0\0\0"
                continue
            # Average premultiplied samples, then un-premultiply for straight alpha.
            pixels += bytes((round(r / a * 255), round(g / a * 255), round(b / a * 255), round(a / (n * n) * 255)))
    return bytes(pixels)


def encode_png(size, rgba):
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

    stride = size * 4
    raw = b"".join(b"\0" + rgba[row * stride : (row + 1) * stride] for row in range(size))
    header = struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)  # 8-bit RGBA
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")


def decode_png(data):
    """Decodes a PNG written by encode_png (RGBA, filter type 0 only) to (size, rgba)."""
    position, idat, size = 8, b"", 0
    while position < len(data):
        (length,) = struct.unpack(">I", data[position : position + 4])
        kind = data[position + 4 : position + 8]
        body = data[position + 8 : position + 8 + length]
        if kind == b"IHDR":
            size = struct.unpack(">I", body[:4])[0]
        elif kind == b"IDAT":
            idat += body
        position += 12 + length
    raw = zlib.decompress(idat)
    stride = size * 4 + 1
    return size, b"".join(raw[row * stride + 1 : (row + 1) * stride] for row in range(size))


def encode_ico(images):
    """images: list of (size, png bytes). PNG entries are valid in .ico since Windows Vista."""
    header = struct.pack("<HHH", 0, 1, len(images))
    offset = 6 + 16 * len(images)
    directory, payload = b"", b""
    for size, png in images:
        dimension = 0 if size >= 256 else size  # 0 means 256 in the .ico directory
        directory += struct.pack("<BBBBHHII", dimension, dimension, 0, 0, 1, 32, len(png), offset + len(payload))
        payload += png
    return header + directory + payload


def decode_ico(data):
    """Returns {size: rgba} for an .ico written by encode_ico."""
    (count,) = struct.unpack("<H", data[4:6])
    images = {}
    for index in range(count):
        entry = data[6 + 16 * index : 22 + 16 * index]
        _, _, _, _, _, _, length, offset = struct.unpack("<BBBBHHII", entry)
        size, rgba = decode_png(data[offset : offset + length])
        images[size] = rgba
    return images


def window_include(rgba):
    lines = [
        "// Generated by tools/make_icon.py; do not edit. Regenerate with",
        "// `python3 tools/make_icon.py`, which also writes resources/kinetra.ico.",
        f"// {WINDOW_SIZE}x{WINDOW_SIZE} pixels, RGBA bytes, rows top to bottom, straight alpha.",
        f"constexpr int window_icon_size = {WINDOW_SIZE};",
        "constexpr unsigned char window_icon_rgba[] = {",
    ]
    stride = WINDOW_SIZE * 4
    for row in range(WINDOW_SIZE):
        lines.append("    " + ",".join(str(value) for value in rgba[row * stride : (row + 1) * stride]) + ",")
    lines.append("};")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="verify the committed files instead of writing")
    arguments = parser.parse_args()

    images = {size: render(size) for size in ICO_SIZES}
    include = window_include(images[WINDOW_SIZE])

    if arguments.check:
        problems = []
        try:
            if decode_ico(ICO_PATH.read_bytes()) != images:
                problems.append(ICO_PATH)
            if decode_png(PNG_PATH.read_bytes()) != (256, images[256]):
                problems.append(PNG_PATH)
            if INC_PATH.read_text() != include:
                problems.append(INC_PATH)
        except (OSError, ValueError, zlib.error, struct.error) as error:
            print(f"Icon files unreadable: {error}", file=sys.stderr)
            return 1
        for path in problems:
            print(f"{path.relative_to(ROOT)} is out of date; run python3 tools/make_icon.py", file=sys.stderr)
        return 1 if problems else 0

    ICO_PATH.parent.mkdir(exist_ok=True)
    ICO_PATH.write_bytes(encode_ico([(size, encode_png(size, images[size])) for size in ICO_SIZES]))
    PNG_PATH.write_bytes(encode_png(256, images[256]))
    INC_PATH.write_text(include)
    return 0


if __name__ == "__main__":
    sys.exit(main())
