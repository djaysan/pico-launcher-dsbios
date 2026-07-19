#!/usr/bin/env python3
"""Convert an image to a Pico Launcher custom icon:
32x32 BMP, 4bpp indexed, uncompressed, with palette color 0 as transparent
(see docs/Customization.md and BmpFileIconData.cpp).

Usage: python3 tools/png2iconbmp.py input.png output.bmp
"""
import struct
import sys

from PIL import Image

SIZE = 32
COLORS = 16  # index 0 = transparent, 1-15 actual colors


def convert(src_path: str, dst_path: str) -> None:
    im = Image.open(src_path).convert("RGBA")
    im.thumbnail((SIZE, SIZE), Image.LANCZOS)
    canvas = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    canvas.paste(im, ((SIZE - im.width) // 2, (SIZE - im.height) // 2))

    mask = canvas.getchannel("A").point(lambda a: 255 if a >= 128 else 0)
    quant = canvas.convert("RGB").quantize(colors=COLORS - 1, method=Image.MEDIANCUT).convert("RGB")

    palette = [(255, 0, 255)]  # index 0: transparent
    pixels = [[0] * SIZE for _ in range(SIZE)]
    qpix, mpix = quant.load(), mask.load()
    for y in range(SIZE):
        for x in range(SIZE):
            if mpix[x, y] == 0:
                continue
            color = qpix[x, y]
            if color not in palette:
                palette.append(color)
            pixels[y][x] = palette.index(color)

    pal_bytes = b"".join(struct.pack("<BBBB", b, g, r, 0) for r, g, b in palette)
    pal_bytes += b"\x00" * (COLORS * 4 - len(pal_bytes))
    pixel_offset = 14 + 40 + COLORS * 4
    # 4bpp: 16 bytes per row (32 px / 2), multiple of 4; rows bottom-up
    rows = []
    for y in range(SIZE - 1, -1, -1):
        row = bytearray(SIZE // 2)
        for x in range(SIZE):
            if x % 2 == 0:
                row[x // 2] |= pixels[y][x] << 4
            else:
                row[x // 2] |= pixels[y][x]
        rows.append(bytes(row))
    pixel_data = b"".join(rows)

    header = struct.pack("<2sIHHI", b"BM", pixel_offset + len(pixel_data), 0, 0, pixel_offset)
    dib = struct.pack("<IiiHHIIiiII", 40, SIZE, SIZE, 1, 4, 0, len(pixel_data), 2835, 2835, COLORS, COLORS)
    with open(dst_path, "wb") as f:
        f.write(header + dib + pal_bytes + pixel_data)
    print(f"{dst_path}: {SIZE}x{SIZE}, 4bpp, {len(palette)} colors")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    convert(sys.argv[1], sys.argv[2])
