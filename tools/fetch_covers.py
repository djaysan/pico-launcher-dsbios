#!/usr/bin/env python3
"""Descarga caratulas faltantes desde libretro-thumbnails (No-Intro) para los
sistemas SIN gamecode (GB, GBC, Mega Drive, etc.) y las instala en
<SD>/_pico/covers/user/<archivo>.bmp — la carpeta que Pico Launcher consulta
por nombre de archivo para cualquier tipo asociado.

(Para GBA usar fetch_covers_gba.py, que aprovecha el gamecode del header.)

Uso: python3 tools/fetch_covers.py <sistema...> [--sd /Volumes/DSPICO] [--dry-run]
     python3 tools/fetch_covers.py gb gbc gen
"""
from __future__ import annotations

import difflib
import json
import os
import re
import sys
import tempfile
import unicodedata
import urllib.parse
import urllib.request

from img2cover import convert

# gamesDir relativo a la SD; exts en minusculas
SYSTEMS = {
    "gb":   {"repo": "Nintendo_-_Game_Boy",                          "gamesDir": "Games/gb",   "exts": (".gb",)},
    "gbc":  {"repo": "Nintendo_-_Game_Boy_Color",                    "gamesDir": "Games/gb",   "exts": (".gbc",)},
    "gen":  {"repo": "Sega_-_Mega_Drive_-_Genesis",                  "gamesDir": "Games/gen",  "exts": (".md", ".gen")},
    "sms":  {"repo": "Sega_-_Master_System_-_Mark_III",              "gamesDir": "Games/sms",  "exts": (".sms",)},
    "gg":   {"repo": "Sega_-_Game_Gear",                             "gamesDir": "Games/gg",   "exts": (".gg",)},
    "nes":  {"repo": "Nintendo_-_Nintendo_Entertainment_System",     "gamesDir": "Games/nes",  "exts": (".nes",)},
    "snes": {"repo": "Nintendo_-_Super_Nintendo_Entertainment_System", "gamesDir": "Games/snes", "exts": (".sfc", ".smc")},
    "ws":   {"repo": "Bandai_-_WonderSwan",                          "gamesDir": "Games/ws",   "exts": (".ws",)},
    "wsc":  {"repo": "Bandai_-_WonderSwan_Color",                    "gamesDir": "Games/ws",   "exts": (".wsc",)},
    "ngp":  {"repo": "SNK_-_Neo_Geo_Pocket",                         "gamesDir": "Games/ngp",  "exts": (".ngp",)},
    "ngc":  {"repo": "SNK_-_Neo_Geo_Pocket_Color",                   "gamesDir": "Games/ngp",  "exts": (".ngc",)},
}

REGION_PREF = ["(Europe", "(USA", "(World", "(Spain", "(Japan"]


def norm(s: str) -> str:
    s = unicodedata.normalize("NFKD", s).encode("ascii", "ignore").decode()
    s = re.sub(r"\(.*?\)", "", s)
    s = re.sub(r"[^a-z0-9]+", " ", s.lower())
    s = re.sub(r"\b(the|a|an|el|la|los|las)\b", " ", s)
    return " ".join(s.split())


def fetch_catalog(repo: str) -> list[str]:
    url = f"https://api.github.com/repos/libretro-thumbnails/{repo}/git/trees/master?recursive=1"
    with urllib.request.urlopen(url, timeout=60) as r:
        tree = json.load(r)["tree"]
    return [
        e["path"].removeprefix("Named_Boxarts/")
        for e in tree
        if e["path"].startswith("Named_Boxarts/") and e["path"].endswith(".png")
    ]


def pick(title: str, by_norm: dict[str, list[str]]) -> str | None:
    key = norm(title)
    candidates = by_norm.get(key)
    if not candidates:
        # prefijo ANTES que fuzzy: difflib confunde numeraciones (Zero 1 vs Zero 4)
        prefixes = [k for k in by_norm if key.startswith(k + " ") or k.startswith(key + " ")]
        if prefixes:
            candidates = by_norm[max(prefixes, key=len)]
    if not candidates:
        close = difflib.get_close_matches(key, by_norm.keys(), n=1, cutoff=0.85)
        if close:
            candidates = by_norm[close[0]]
    if not candidates:
        return None
    for p in REGION_PREF:
        for c in candidates:
            if p in c:
                return c
    return candidates[0]


def main() -> None:
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    sd = "/Volumes/DSPICO"
    if "--sd" in sys.argv:
        sd = sys.argv[sys.argv.index("--sd") + 1]
    dry = "--dry-run" in sys.argv
    if not args or any(a not in SYSTEMS for a in args):
        sys.exit(__doc__ + "\nSistemas: " + ", ".join(SYSTEMS))

    covers_user = os.path.join(sd, "_pico", "covers", "user")
    os.makedirs(covers_user, exist_ok=True)
    have = {f for f in os.listdir(covers_user) if not f.startswith("._")}

    ok, fail = [], []
    for system in args:
        cfg = SYSTEMS[system]
        games_dir = os.path.join(sd, cfg["gamesDir"])
        if not os.path.isdir(games_dir):
            continue
        pending = [
            f for f in sorted(os.listdir(games_dir))
            if not f.startswith("._") and f.lower().endswith(cfg["exts"]) and f + ".bmp" not in have
        ]
        if not pending:
            print(f"{system}: nada que hacer")
            continue

        print(f"{system}: descargando catálogo {cfg['repo']}...")
        catalog = fetch_catalog(cfg["repo"])
        by_norm: dict[str, list[str]] = {}
        for name in catalog:
            by_norm.setdefault(norm(name[:-4]), []).append(name)
        print(f"{system}: {len(catalog)} boxarts, {len(pending)} juegos sin carátula")

        raw_base = f"https://raw.githubusercontent.com/libretro-thumbnails/{cfg['repo']}/master/Named_Boxarts/"
        for f in pending:
            match = pick(f.rsplit(".", 1)[0], by_norm)
            if not match:
                fail.append((system, f, "sin match en catálogo"))
                continue
            if dry:
                ok.append((system, f, match))
                continue
            try:
                url = raw_base + urllib.parse.quote(match)
                with urllib.request.urlopen(url, timeout=60) as r, tempfile.NamedTemporaryFile(suffix=".png") as tmp:
                    tmp.write(r.read())
                    tmp.flush()
                    convert(tmp.name, os.path.join(covers_user, f + ".bmp"))
                ok.append((system, f, match))
            except Exception as e:  # noqa: BLE001 — reportar y seguir
                fail.append((system, f, str(e)))

    print(f"\n✓ {len(ok)} caratulas{' (dry-run)' if dry else ' instaladas'}:")
    for s, f, m in ok:
        print(f"  [{s}] {f}  ←  {m}")
    if fail:
        print(f"\n✗ {len(fail)} sin resolver:")
        for s, f, why in fail:
            print(f"  [{s}] {f}: {why}")


if __name__ == "__main__":
    main()
