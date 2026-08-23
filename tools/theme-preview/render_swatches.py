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
    """One bottom screen: side app bar, a grid of game cells, a selected cell."""
    im = Image.new('RGB', (W, H), rgb(c['surfaceBright']))
    d = ImageDraw.Draw(im)
    f = font(9)

    # left app bar with its buttons
    d.rectangle([0, 0, 33, H - 1], fill=rgb(c['surfaceContainerHighest']))
    for i, y in enumerate((13, 44, 79, 111, 144, 175)):
        sel = i == 4                       # the guides button, tinted like a filter
        col = rgb(c['primary']) if sel else rgb(c['onSurfaceVariant'])
        d.ellipse([11, y - 6, 23, y + 6], outline=col, width=2)

    # game grid
    for r in range(3):
        for col in range(4):
            x = 44 + col * 51
            y = 16 + r * 58
            selected = (r == 0 and col == 1)
            box = [x, y, x + 42, y + 42]
            d.rounded_rectangle(box, 6, fill=rgb(c['mainIconBg']),
                                outline=rgb(c['primary']) if selected else None,
                                width=2 if selected else 0)
            d.rounded_rectangle([x + 9, y + 9, x + 33, y + 33], 3,
                                fill=rgb(c['surfaceBright']))
            d.text((x, y + 45), 'Game', font=f, fill=rgb(c['onSurfaceVariant']))

    # count pill, the launcher's top-left chip
    d.rounded_rectangle([44, 172, 104, 186], 7, fill=rgb(c['secondaryContainer']))
    d.text((52, 175), '478 games', font=f, fill=rgb(c['onSecondaryContainer']))
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
