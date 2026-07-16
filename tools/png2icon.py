#!/usr/bin/env python3
"""Prepara el icono del banner del cartucho NDS a partir de cualquier imagen.

ndstool (BlocksDS) acepta PNG con alpha para `-b`, pero exige <=16 colores en
total. Este script encaja la imagen en 32x32, umbraliza el alpha (>=128 opaco)
y cuantiza los pixeles opacos a 15 colores; los transparentes se colapsan en
un unico color con alpha 0. ndstool asigna el indice 0 (transparente en DS) a
ese color y los 15 restantes a los indices 1-15.

Uso: python3 tools/png2icon.py entrada.png [salida.png]
"""
import sys

from PIL import Image

SIZE = 32
OPAQUE_COLORS = 15  # + 1 transparente = 16 en total


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
    assert len(opaque) <= OPAQUE_COLORS, f"quedaron {len(opaque)} colores opacos"

    out.save(dst_path)
    print(f"{dst_path}: {SIZE}x{SIZE}, {len(opaque)} colores opacos + transparencia")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    convert(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else "icon.png")
