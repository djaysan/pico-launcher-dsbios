#!/usr/bin/env python3
"""Render candidate material themes as DS-sized mockups.

Feeds seeds through dump_scheme (which is the launcher's own palette code
built for the host) and draws each result roughly the way the launcher lays
out its bottom screen, so a theme can be judged before it goes near a card.

  ./run.sh            # build dump_scheme, then render every candidate here
"""
import json, os, subprocess, sys
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
DUMP = os.environ.get('DUMP_SCHEME', os.path.join(HERE, 'dump_scheme'))

# name, seed, mode
# The four themes shipped in theme-remaster/out/. Keep in sync with their
# theme.json files - seed is primaryColor, mode is darkTheme/pureBlack.
CANDIDATES = [
    ('RML Black',    'FFC93C', 'black'),
    ('RML Dark',     'FFC93C', 'dark'),
    ('RML Light',    'FFC93C', 'light'),
    ('RML Daylight', '5A6672', 'light'),
]

W, H, SCALE = 256, 192, 2
PAD, GAP = 14, 12

def rgb(h):
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))

def scheme(seed, mode, name):
    out = subprocess.run([DUMP, seed, mode, name], capture_output=True, text=True)
    if out.returncode != 0:
        sys.exit(f'dump_scheme failed for {seed} {mode}: {out.stderr}')
    return json.loads(out.stdout)

def font(sz):
    for p in ('/System/Library/Fonts/Supplemental/Arial Bold.ttf',
              '/System/Library/Fonts/Helvetica.ttc'):
        if os.path.exists(p):
            try:
                return ImageFont.truetype(p, sz)
            except OSError:
                pass
    return ImageFont.load_default()

def mock(c):
    """One bottom screen, using the colours the launcher actually uses.

    The background is inverseOnSurface (GFX_PLTT_BG_MAIN[0]) - NOT surfaceBright,
    which is only the icon cell face. MaterialIconGridItemView blends the cell
    between inverseOnSurface as back and surfaceBright as front, swapping the
    front for mainIconBg while focused.
    """
    bg = rgb(c['inverseOnSurface'])
    im = Image.new('RGB', (W, H), bg)
    d = ImageDraw.Draw(im)
    f = font(9)

    # app bar down the left, icons only - it sits straight on the background
    for i, y in enumerate((13, 44, 79, 111, 144, 175)):
        sel = i == 4
        col = rgb(c['primary']) if sel else rgb(c['onSurfaceVariant'])
        d.ellipse([11, y - 6, 23, y + 6], outline=col, width=2)

    # icon cells
    for r in range(3):
        for col in range(4):
            x, y = 44 + col * 51, 16 + r * 58
            focused = (r == 0 and col == 1)
            face = rgb(c['mainIconBg']) if focused else rgb(c['surfaceBright'])
            d.rounded_rectangle([x, y, x + 42, y + 42], 6, fill=face)
            d.rounded_rectangle([x + 9, y + 9, x + 33, y + 33], 3, fill=bg)
            d.text((x, y + 45), 'Game', font=f, fill=rgb(c['onSurfaceVariant']))
    return im

def main():
    cards = []
    for name, seed, mode in CANDIDATES:
        cards.append((name, seed, mode, mock(scheme(seed, mode, name)['colors'])))

    cw, ch = W * SCALE, H * SCALE + 30
    cols = 2
    rows = (len(cards) + cols - 1) // cols
    sheet = Image.new('RGB', (PAD * 2 + cols * cw + (cols - 1) * GAP,
                              PAD * 2 + rows * ch + (rows - 1) * GAP), (28, 28, 30))
    sd = ImageDraw.Draw(sheet)
    lf = font(20)
    for i, (name, seed, mode, im) in enumerate(cards):
        x = PAD + (i % cols) * (cw + GAP)
        y = PAD + (i // cols) * (ch + GAP)
        sd.text((x, y), f'{name}   #{seed} {mode}', font=lf, fill=(235, 235, 240))
        sheet.paste(im.resize((cw, ch - 30), Image.NEAREST), (x, y + 26))
    out = os.path.join(HERE, 'candidates.png')
    sheet.save(out)
    print(out)

if __name__ == '__main__':
    main()
