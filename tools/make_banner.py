#!/usr/bin/env python3
"""Generate banner.bnr files (NDS banner v1, 0x840 bytes) for Pico Launcher
folders: a 32x32 icon + a title that replaces the folder name.

The icon can come from:
  --from-nds ROM.nds   copies bitmap+palette from another ROM's banner (lossless)
  --from-image img.png quantizes an image to 15 colors + transparency (alpha)

Usage:
  python3 tools/make_banner.py --from-nds emulator.nds "Title" output.bnr
  python3 tools/make_banner.py --from-image logo.png "Title" output.bnr
"""
from __future__ import annotations

import struct
import sys

BANNER_SIZE = 0x840  # v1: 0x20 header + 0x200 icon + 0x20 palette + 6 titles


def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc


def icon_from_nds(path: str) -> tuple[bytes, bytes]:
    with open(path, "rb") as f:
        data = f.read()
    off = struct.unpack_from("<I", data, 0x68)[0]
    if off == 0 or off + 0x240 > len(data):
        raise ValueError(f"{path}: no banner")
    return data[off + 0x20 : off + 0x220], data[off + 0x220 : off + 0x240]


def icon_from_image(path: str) -> tuple[bytes, bytes]:
    from PIL import Image

    im = Image.open(path).convert("RGBA")
    im.thumbnail((32, 32), Image.LANCZOS)
    canvas = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    canvas.paste(im, ((32 - im.width) // 2, (32 - im.height) // 2))

    mask = canvas.getchannel("A").point(lambda a: 255 if a >= 128 else 0)
    quant = canvas.convert("RGB").quantize(colors=15, method=Image.MEDIANCUT).convert("RGB")

    pal_colors: list[tuple[int, int, int]] = []
    pixels = bytearray(32 * 32)
    qpix, mpix = quant.load(), mask.load()
    for y in range(32):
        for x in range(32):
            if mpix[x, y] == 0:
                continue
            c = qpix[x, y]
            if c not in pal_colors:
                pal_colors.append(c)
            pixels[y * 32 + x] = pal_colors.index(c) + 1

    bitmap = bytearray(0x200)
    for i in range(1024):
        tile, off = i // 64, i % 64
        x = (tile % 4) * 8 + off % 8
        y = (tile // 4) * 8 + off // 8
        idx = pixels[y * 32 + x]
        if i % 2 == 0:
            bitmap[i // 2] |= idx
        else:
            bitmap[i // 2] |= idx << 4

    palette = bytearray(0x20)
    struct.pack_into("<H", palette, 0, 0x7C1F)  # index 0: magenta (transparent)
    for i, (r, g, b) in enumerate(pal_colors[:15]):
        struct.pack_into("<H", palette, (i + 1) * 2, (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10))
    return bytes(bitmap), bytes(palette)


def make_banner(bitmap: bytes, palette: bytes, title: str) -> bytes:
    assert len(bitmap) == 0x200 and len(palette) == 0x20
    banner = bytearray(BANNER_SIZE)
    struct.pack_into("<H", banner, 0x00, 0x0001)
    banner[0x20:0x220] = bitmap
    banner[0x220:0x240] = palette
    encoded = title.encode("utf-16-le")[:0xFE]
    for lang in range(6):  # Japanese..Spanish, same text
        base = 0x240 + lang * 0x100
        banner[base : base + len(encoded)] = encoded
    struct.pack_into("<H", banner, 0x02, crc16(banner[0x20:BANNER_SIZE]))
    return bytes(banner)


if __name__ == "__main__":
    if len(sys.argv) != 5 or sys.argv[1] not in ("--from-nds", "--from-image"):
        sys.exit(__doc__)
    mode, src, title, dst = sys.argv[1:]
    bitmap, palette = icon_from_nds(src) if mode == "--from-nds" else icon_from_image(src)
    with open(dst, "wb") as f:
        f.write(make_banner(bitmap, palette, title))
    print(f"{dst}: banner v1, title \"{title}\"")
