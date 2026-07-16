#!/usr/bin/env python3
"""Convierte cualquier imagen a una caratula de Pico Launcher.

Formato (ver arm9/source/romBrowser/FileType/BmpHeader.h y BmpFileCover.cpp):
BMP de 128x96, 8bpp indexado, sin compresion, DIB de 40 bytes, clrUsed=256.
El launcher solo muestra los 106x96 de la izquierda, asi que el arte se encaja
ahi (proporcion conservada, relleno negro) y las columnas 106-127 quedan negras.

Uso: python3 tools/img2cover.py entrada.png salida.bmp
"""
import struct
import sys

from PIL import Image

W, H = 128, 96
VISIBLE_W = 106


def convert(src_path: str, dst_path: str) -> None:
    im = Image.open(src_path).convert("RGB")
    im.thumbnail((VISIBLE_W, H), Image.LANCZOS)

    canvas = Image.new("RGB", (W, H), (0, 0, 0))
    canvas.paste(im, ((VISIBLE_W - im.width) // 2, (H - im.height) // 2))

    quant = canvas.quantize(colors=256, method=Image.MEDIANCUT, dither=Image.FLOYDSTEINBERG)
    pal = quant.getpalette()[: 256 * 3]
    pixels = quant.tobytes()

    pal_bytes = b"".join(
        struct.pack("<BBBB", pal[i * 3 + 2], pal[i * 3 + 1], pal[i * 3], 0) for i in range(256)
    )
    pixel_offset = 14 + 40 + len(pal_bytes)
    # filas de abajo hacia arriba (bottom-up), 128 bytes/fila (multiplo de 4)
    pixel_data = b"".join(pixels[y * W : (y + 1) * W] for y in range(H - 1, -1, -1))

    header = struct.pack("<2sIHHI", b"BM", pixel_offset + len(pixel_data), 0, 0, pixel_offset)
    dib = struct.pack("<IiiHHIIiiII", 40, W, H, 1, 8, 0, len(pixel_data), 2835, 2835, 256, 0)

    with open(dst_path, "wb") as f:
        f.write(header + dib + pal_bytes + pixel_data)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    convert(sys.argv[1], sys.argv[2])
    print(f"{sys.argv[2]}: {W}x{H}, 8bpp, 256 colores")
