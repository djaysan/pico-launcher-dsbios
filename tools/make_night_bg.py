#!/usr/bin/env python3
"""Genera las variantes nocturnas (*_night.bin) de los fondos de un tema
custom de Pico Launcher. Los .bin son volcados crudos 256x192 en BGR555;
la variante nocturna oscurece y enfria los colores.

Uso: python3 tools/make_night_bg.py <carpeta-del-tema>
     python3 tools/make_night_bg.py "/Volumes/DSPICO/_pico/themes/Basic Gray"
"""
import os
import struct
import sys

W, H = 256, 192


def night_tint(r: int, g: int, b: int) -> tuple[int, int, int]:
    # oscurecer y correr hacia el azul: mood nocturno
    nr = int(r * 0.40)
    ng = int(g * 0.48)
    nb = min(31, int(b * 0.62) + 3)
    return nr, ng, nb


def convert(src_path: str, dst_path: str) -> None:
    with open(src_path, "rb") as f:
        data = bytearray(f.read())
    if len(data) < W * H * 2:
        raise ValueError(f"{src_path}: tamaño inesperado ({len(data)} bytes)")
    for i in range(0, W * H * 2, 2):
        v = data[i] | (data[i + 1] << 8)
        r, g, b = v & 31, (v >> 5) & 31, (v >> 10) & 31
        nr, ng, nb = night_tint(r, g, b)
        nv = (v & 0x8000) | nr | (ng << 5) | (nb << 10)
        data[i] = nv & 0xFF
        data[i + 1] = nv >> 8
    with open(dst_path, "wb") as f:
        f.write(data)
    print(f"{dst_path} generado")


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
        sys.exit(f"No hay topbg.bin/bottombg.bin en {theme_dir}")
