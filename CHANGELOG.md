# Changelog

## Enhanced fork

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