# Game Data File
Pico Launcher Enhanced stores per-game data (favorites, launch counts, play time — see [Enhanced.md](Enhanced.md)) in `/_pico/gamedata.json`. This document specifies the format for external tool authors.

Regular users never need to edit this file. Deleting it simply resets all favorites and statistics.

## Example
```json
{
  "games": {
    "Some Game.nds": {
      "gameCode": "ABCE",
      "favorite": true,
      "launchCount": 12,
      "playMinutes": 340,
      "lastPlayed": "2026-07-16 21:03",
      "path": "/Games/nds/Some Game.nds"
    },
    "tetris.gb": {
      "launchCount": 2,
      "lastPlayed": "2026-07-10 18:40",
      "path": "/Games/gb/tetris.gb"
    }
  },
  "sessionGame": "Some Game.nds",
  "sessionGameCode": "ABCE",
  "sessionStart": "2026-07-16 21:03"
}
```

## The `games` object
Each key in `games` is a file name (not a path). All keys inside an entry are optional — the launcher omits any key holding its default value:

| Key | Type | Written when | Meaning |
|---|---|---|---|
| `gameCode` | string | non-empty | Internal game code from the NDS/GBA header. Only stored when it is printable ASCII (homebrew ROMs can hold garbage there). |
| `favorite` | bool | `true` | Marked as favorite. Absent means not a favorite — `false` is never written. |
| `launchCount` | number | > 0 | How many times the game was launched. |
| `playMinutes` | number | > 0 | Accumulated play time in minutes (approximate — see below). |
| `lastPlayed` | string | non-empty | `"YYYY-MM-DD HH:MM"`, 24-hour clock. Lexicographic order equals chronological order, so tools can sort these as plain strings. |
| `path` | string | non-empty | Full path of the file at its last launch. Used by the recently played panel to navigate back to the game. |

An entry whose `favorite` is false and whose `launchCount` and `playMinutes` are both 0 is pruned on the next write. Deleting a game through the launcher also removes its entry.

## Session keys (root level)
While a play session is open, the root object holds:

| Key | Written when | Meaning |
|---|---|---|
| `sessionGame` | session open | File name of the launched game. |
| `sessionGameCode` | session open and code known | Its game code. |
| `sessionStart` | session open | Launch time, `"YYYY-MM-DD HH:MM"`. |

A session opens when a game is launched and closes at the next launcher boot. On boot, the elapsed time since `sessionStart` is added to the game's `playMinutes` if it is between 1 minute and 6 hours (longer means the console was powered off, not playing), and the session keys are removed.

## Identity and self-healing
- Lookups prefer `gameCode` and fall back to the entry key (file name). Both comparisons are case-insensitive.
- When a game with a known code is used again under a different file name, its entry is re-keyed to the new name automatically — renaming a ROM does not lose its data.
- An entry created before the game's code was known gets its `gameCode` added the first time the code is read.
- Consequence: two ROMs with the same game code (for example a ROM hack and its base game, or copies in two folders) share one entry, and only the most recently used file name is stored.

## Limits
Values longer than these are truncated (lengths in characters):

- entry key / `sessionGame` (file name): 96
- `gameCode`: 8
- `lastPlayed` / `sessionStart`: 20
- `path`: 256

Years in `lastPlayed` are written as `20YY`; dates before the year 2000 do not parse and are ignored for play-time accounting.

## Rules for external tools
- **Unknown keys are not preserved.** The launcher rewrites the whole file on every change and serializes only the keys listed above. Do not store tool-specific data in this file.
- The file is pretty-printed JSON and is replaced in full on every save. Entry order is not meaningful and not preserved.
- If the file cannot be parsed, the launcher starts with no data and overwrites the file on its next save — a tool that writes invalid JSON effectively wipes everything.
- Keep `gameCode` values unique: entries sharing a code are merged when the file is loaded, with the later entry's values overwriting the earlier one's.
- The launcher parses the file with a bounded memory budget of 96 KB (about three times the file size); files larger than roughly 32 KB may fail to load. In practice this fits several hundred entries.
