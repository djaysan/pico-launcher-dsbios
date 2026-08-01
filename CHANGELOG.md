# Changelog

## Enhanced fork

### [enhanced-v1.5.0]

#### Fixed
- Favorites and completed marks now belong to the ROM file, so the browser filter and the top screen always agree. Filtering by favorites could come up empty, and on a renamed ROM pressing X unmarked the game instead of marking it
- ROM hacks no longer inherit their base game's favorite, completed mark and play time, and two copies of a game are tracked separately (fixes #1 as well as #7)
- Empty folders no longer reappear while browsing: probing a folder with a filter active read every ROM in it and ran out of its budget, so every folder checked afterwards was shown again
- Saving game data is now atomic and refuses to overwrite a file it could not read, so an interrupted save or a corrupt file can no longer wipe every favorite and play stat
- The focus highlight on app bar buttons is genuinely visible now; the v1.3.0 attempt was too subtle to see on hardware
- Going up a folder lands on the folder you just left instead of jumping back to the first entry
- The delete button is dimmed while a folder is highlighted, since only games can be deleted
- Favorites and recents entries whose file is gone no longer drop you at the card root when activated
- ROM file names longer than 96 bytes are refused instead of being silently truncated, which used to merge two files with a long shared prefix into one entry

### [enhanced-v1.4.0]

#### Fixed
- The game count and launch info on the top screen are readable on any theme: they sit on launcher-drawn chips instead of relying on the theme's artwork for contrast, and custom themes can position or hide them (issue #4)

### [enhanced-v1.3.0]

#### Added
- Favorites panel: hold the heart button to see all favorites from every folder and jump to any of them
- Marking a favorite or completed game now stores its path, so the favorites panel works for games never launched

#### Fixed
- The focus highlight on app bar buttons is now clearly visible while moving with the dpad
- Homebrew ROMs carrying the `####` placeholder game code no longer share one gamedata entry (favorites, completed marks and play time no longer bleed between them)

### [enhanced-v1.2.0]

#### Added
- Screen brightness control for the DS Lite: a Light row in the display settings sheet with the four backlight levels, remembered across boots and active in-game

### [enhanced-v1.1.0]

#### Added
- Completed games: hold X on a game to mark it as completed, shown as a green check on the top screen
- Completed filter with a check button in the app bar (green while active)
- Completed count in the statistics panel

### [enhanced-v1.0.0]

#### Added
- Game count of the current folder on the top screen
- Random game launch with the SELECT button
- Favorites toggled with the X button, shown as a heart on the top screen
- Favorites filter with a heart button in the app bar (red while active)
- Recently played panel (clock button in the app bar); tapping an entry navigates to the game
- Statistics panel (START button)
- Per-game launch tracking (launch count and last-played date on the top screen)
- Approximate play time per game, with totals in the statistics panel
- Game deletion (trash button with confirmation); removes the ROM and its save file
- Per-folder background music with a `bgm.bcstm` file inside a folder
- Optional time-of-day theme backgrounds (`topbg_night.bin`/`bottombg_night.bin`, shown 20:00–6:59)
- Persistent per-game data in `/_pico/gamedata.json`, keyed by gamecode with filename fallback and self-healing renames
- Desktop helper tools for covers, banners, icons and night backgrounds (see `tools/`)

## [Unreleased]

### Added
- Support for custom BMP icons for games and folders - by @tasken
- Support for custom NDS banners (custom titles, subtitles and animated icons) for games and folders - by @tasken
- Theme selector

### Fixed
- Top screen cover is now displayed/hidden correctly when placed partially or fully off-screen
- DSi banners with missing DSi part now fall back to the DS icon

## [v1.3.0] - 18 Apr 2026

### Added
- Ability to set the position of the top screen cover image in custom themes
- Support for fast scrolling with the L and R buttons in coverflow display mode
- Support for touch input

### Fixed
- Use after free bug with the texture load request in Label3DView. This occurred for example when spamming B in banner list mode.

## [v1.2.0] - 29 Mar 2026

### Added
- Support for cheats with Pico Loader API v3
- Hide files/dirs with hidden attribute, or with a name starting with a period
- New customization options for custom themes
    - Position of elements on the top screen
    - Text colors
    - Blend colors

### Changed
- File name on the top screen now uses marquee when too long

### Fixed
- Improve error handling for banners to better detect if a rom has a valid banner

## [v1.1.0] - 11 Jan 2026

### Added
- Support for Pico Loader API v2. This makes it possible to return to Pico Launcher from supported homebrew applications.

## [v1.0.0] - 25 Nov 2025
- Initial release