#!/usr/bin/env python3
"""Generate the night variants (*_night.bin) of a Pico Launcher custom
theme's backgrounds. The .bin files are raw 256x192 BGR555 dumps; the night
variant darkens and cools the colors.

Usage: python3 tools/make_night_bg.py <theme-folder>
       python3 tools/make_night_bg.py "/Volumes/DSPICO/_pico/themes/Basic Gray"
"""
import os
import struct
import sys

W, H = 256, 192


def night_tint(r: int, g: int, b: int) -> tuple[int, int, int]:
    # darken and shift toward blue: night mood
    nr = int(r * 0.40)
    ng = int(g * 0.48)
    nb = min(31, int(b * 0.62) + 3)
    return nr, ng, nb


def convert(src_path: str, dst_path: str) -> None:
    with open(src_path, "rb") as f:
        data = bytearray(f.read())
    if len(data) < W * H * 2:
        raise ValueError(f"{src_path}: unexpected size ({len(data)} bytes)")
    for i in range(0, W * H * 2, 2):
        v = data[i] | (data[i + 1] << 8)
        r, g, b = v & 31, (v >> 5) & 31, (v >> 10) & 31
        nr, ng, nb = night_tint(r, g, b)
        nv = (v & 0x8000) | nr | (ng << 5) | (nb << 10)
        data[i] = nv & 0xFF
        data[i + 1] = nv >> 8
    with open(dst_path, "wb") as f:
        f.write(data)
    print(f"{dst_path} written")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    theme_dir = sys.argv[1]
    made = 0
    for name in ("topbg", "bottombg"):
        src = os.path.join(theme_dir, f"{name}.bin")
        if os.path.isfile(src):
            convert(src, os.path.join(theme_dir, f"{name}_night.bin"))
            made += 1
    if made == 0:
        sys.exit(f"No topbg.bin/bottombg.bin in {theme_dir}")
