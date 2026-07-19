# Enhanced Features
This fork adds a number of quality-of-life features on top of Pico Launcher. This document describes each of them.

## Controls
These controls are available in the rom browser, on top of the standard ones (see [Usage](Usage.md)):

| Input | Action |
|---|---|
| X (short press) | Toggle favorite for the highlighted game |
| X (hold ~half a second) | Toggle completed for the highlighted game |
| SELECT | Launch a random game from the current folder |
| START | Open the statistics panel |
| Heart button (app bar) | Toggle the favorites filter (the heart turns red while active) |
| Check button (app bar) | Toggle the completed filter (the check turns green while active) |
| Clock button (app bar) | Open the recently played panel |
| Trash button (app bar) | Delete the highlighted game (X confirms, A or B cancels) |

## Game count
The top-left of the top screen shows how many games the current folder contains (e.g. `12 games`). Only games are counted, not folders or other files.

## Favorites
Press X on a highlighted game to mark it as a favorite (press again to unmark). Favorites show a small heart on the top screen when highlighted. Games are remembered by their internal game code where possible, so renaming or moving a ROM keeps its favorite mark.

## Completed games
Hold X on a highlighted game for about half a second to mark it as completed (hold again to unmark). Completed games show a small green check on the top screen when highlighted, next to the heart. Like favorites, the mark follows the game's internal game code across renames.

## Favorites and completed filters
The heart button in the app bar filters the browser down to favorites; the heart is drawn red while the filter is active. The check button next to it filters down to completed games and turns green while active. With both filters on, only games that are favorite *and* completed remain. The filters apply per folder — folders themselves always stay visible.

Limitations: the filters match by file name, and duplicate copies of the same game share a single entry (see [GameData.md](GameData.md)). A second copy of a favorite in another folder shows the heart, but only the most recently used copy passes the filter. ROM hacks share the game code of their base game, so they also share its favorite mark and statistics.

## Random game
Press SELECT to launch a random game from the folder you are currently viewing. With the favorites filter active, it picks a random favorite.

## Launch tracking and play time
Every launch is recorded automatically. The top-right of the top screen shows the highlighted game's launch count together with its total play time (`3x 2h05`), or with the date it was last played (`3x 16/07`, day/month) when no play time has been recorded yet.

Play time is approximate: a session starts when a game is launched and ends the next time the launcher boots. Because of that:
- Sessions longer than 6 hours are discarded — that was a power-off, not a play session.
- Time spent in sleep mode counts as play time.
- A session is lost if the console is powered off without booting back into the launcher.

## Recently played
The clock button in the app bar opens a list of up to 20 recently played games, most recent first, each with the date and time it was last played. Tap an entry (or highlight it and press A) to jump to that game's folder with the game preselected. Press B to close the panel.

## Statistics
Press START to open a summary panel: how many games you have played, favorited and completed, total launches and total play time, your top 3 most launched games, and the last game you played. Press B or START to close it.

## Deleting games
The trash button in the app bar deletes the highlighted game. A confirmation sheet opens first: press **X** to confirm, or A or B to cancel. Only games can be deleted, not folders.

Deleting a game also deletes its save file (same name with a `.sav` extension, next to the ROM) and removes the game's entry from `gamedata.json`. Note that saves are matched by name without the extension: if `Game.gba` and `Game.nds` sit in the same folder, they share `Game.sav`, and deleting either game deletes it.

## Per-folder music
Place a `bgm.bcstm` file directly inside a folder to give it its own background music. It uses the same DSP-ADPCM `.bcstm` format as theme music (see [Themes](Themes.md)) and supports looping. The music starts when you enter the folder and switches back to the theme music when you leave. Each folder is checked independently — subfolders do not inherit their parent's music.

The `bgm.bcstm` file itself is not shown in the rom browser (file extensions without an association are hidden).

## Time-of-day theme backgrounds
Custom themes can provide night variants of their backgrounds: place `topbg_night.bin` and/or `bottombg_night.bin` next to `topbg.bin` and `bottombg.bin` in the theme folder (same 256x192, 15 bpp format). Between 20:00 and 6:59 the night variants are used when present. The time is checked when the launcher starts. Delete the `_night` files to disable the effect.

`tools/make_night_bg.py` can generate night variants from a theme's existing backgrounds — see [Tools.md](Tools.md).

## Data storage
Favorites, completed marks, launch counts, play time and the recents list are all stored in a single file, `/_pico/gamedata.json`, written by the launcher itself. Games are identified by their internal game code with the file name as fallback, so renaming a ROM does not lose its data. Deleting the file resets all favorites and statistics.

The file format is documented in [GameData.md](GameData.md) for anyone writing external tools.
