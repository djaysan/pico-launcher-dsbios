#!/usr/bin/env python3
"""Prepare the NDS cartridge banner icon from any image.

ndstool (BlocksDS) accepts a PNG with alpha for `-b`, but requires <=16 colors
in total. This script fits the image into 32x32, thresholds the alpha (>=128
opaque) and quantizes the opaque pixels to 15 colors; transparent ones are
collapsed into a single color with alpha 0. ndstool assigns index 0
(transparent on DS) to that color and the remaining 15 colors to indices 1-15.

Usage: python3 tools/png2icon.py input.png [output.png]
"""
import sys

from PIL import Image

SIZE = 32
OPAQUE_COLORS = 15  # + 1 transparent = 16 total


def convert(src_path: str, dst_path: str) -> None:
    im = Image.open(src_path).convert("RGBA")

    im.thumbnail((SIZE, SIZE), Image.LANCZOS)
    canvas = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    canvas.paste(im, ((SIZE - im.width) // 2, (SIZE - im.height) // 2))

    mask = canvas.getchannel("A").point(lambda a: 255 if a >= 128 else 0)
    quant = canvas.convert("RGB").quantize(
        colors=OPAQUE_COLORS, method=Image.MEDIANCUT, dither=Image.FLOYDSTEINBERG
    ).convert("RGB")

    out = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    out.paste(quant, mask=mask)

    colors = out.getcolors(SIZE * SIZE)
    opaque = [c for _, c in colors if c[3] == 255]
    assert len(opaque) <= OPAQUE_COLORS, f"{len(opaque)} opaque colors remain"

    out.save(dst_path)
    print(f"{dst_path}: {SIZE}x{SIZE}, {len(opaque)} opaque colors + transparency")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    convert(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else "icon.png")
