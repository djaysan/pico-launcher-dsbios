# Themes
Using themes, the look and feel of Pico Launcher can be customized. Themes are placed in subfolders of the `/_pico/themes` folder. For example `/_pico/themes/my_theme`.

## JSON file
Each theme has a `theme.json` file with information about the theme.

- **type** - Type of theme. Currently `material`, `custom` and `dsbios` are supported. See below for information about each type.
- **name** - The name of the theme.
- **description** - Description of the theme.
- **author** - Author of the theme.
- **primaryColor** - Material Design 3 primary color to use. `r`, `g` and `b` are provided in range 0-255.
- **darkTheme** - When `true`, a dark Material Design 3 palette will be used.

- **colors** - Optional. Overrides individual Material Design 3 roles, so a theme can pin exact colors instead of accepting everything the `primaryColor` seed derives. Any role left out keeps its derived value. Values are `"#RRGGBB"`; a malformed value is ignored rather than guessed at, so a typo leaves that role on its derived color instead of turning it black.

  Roles: `primary`, `onPrimary`, `secondaryContainer`, `onSecondaryContainer`, `tertiary`, `onTertiary`, `tertiaryContainer`, `onTertiaryContainer`, `surfaceBright`, `inverseOnSurface`, `onSurface`, `onSurfaceVariant`, `mainIconBg`, `surfaceContainerHighest`, `scrim`, `outline`.

  Note that `inverseOnSurface` is the background color, not `surfaceBright`.

### Example
```json
{
    "type": "material",
    "name": "Theme name",
    "description": "Theme description here.",
    "author": "Author Name",
    "primaryColor": {
        "r": 149,
        "g": 143,
        "b": 237
    },
    "darkTheme": true
}
```

## Material type
![Horizontal display mode with custom theme](images/Horizontal.png)

The `material` type theme is a pure Material Design 3 theme. It will be fully themed based on the `primaryColor` and `darkTheme` settings from the `theme.json`. In coverflow mode, this theme type uses a Material Design 3 style carousel.

## Custom type
![Horizontal display mode with custom theme](images/HorizontalCustom.png)

The `custom` type theme is much more customizable, compared to the `material` type theme.
Note that the `primaryColor` and `darkTheme` settings from the `theme.json` are still used to color some parts of the UI.

The following additional files are needed:
| Files                                                        | Size                 | Format                   | Description                                                    |
|--------------------------------------------------------------|----------------------|--------------------------|----------------------------------------------------------------|
| bannerListCell.bin<br>bannerListCellPltt.bin                 | 256x49 (209x49 used) | A3I5<br>32 color palette | Unselected item background for banner list mode.               |
| bannerListCellSelected.bin<br>bannerListCellSelectedPltt.bin | 256x49 (209x49 used) | A3I5<br>32 color palette | Selected item background for banner list mode.                 |
| bottombg.bin                                                 | 256x192              | 15 bpp bitmap            | Bottom screen background.                                      |
| gridcell.bin<br>gridcellPltt.bin                             | 64x48 (48x48 used)   | A3I5<br>32 color palette | Unselected item background for grid modes.                     |
| gridcellSelected.bin<br>gridcellPlttSelected.bin             | 64x48 (48x48 used)   | A3I5<br>32 color palette | Selected item background for grid modes.                       |
| scrim.bin<br>scrimPltt.bin                                   | 8x42                 | A5I3<br>8 color palette  | Background for the toolbar. Intended to be a translucent fade. |
| topbg.bin                                                    | 256x192              | 15 bpp bitmap            | Top screen background.                                         |

These files can be created, for example, using [NitroPaint](https://github.com/Garhoogin/NitroPaint).

The top screen background should include a box in which the banner text and icon of the selected item will be shown.

### Additional JSON properties
Custom themes support additional properties in the `theme.json` file to allow for more customization.

- **topIcon** - Properties of the icon displayed on the top screen.
- **topBannerTextLine0** - Properties of the first banner text line displayed on the top screen.
- **topBannerTextLine1** - Properties of the second banner text line displayed on the top screen.
- **topBannerTextLine2** - Properties of the third banner text line displayed on the top screen.
- **topFileNameText** - Properties of the file name text displayed on the top screen.
- **topCover** - Properties of the cover image displayed on the top screen.
- **topGameCount** - Properties of the game count shown on the top screen. `position` is the top-left corner of its pill. Set `hidden` to `true` to remove it entirely.
- **topLaunchInfo** - Properties of the launch info shown on the top screen (play count and last played, plus the favorite and completed markers). `position` is the top-right corner of its pill, which grows to the left. Set `hidden` to `true` to remove it entirely.
- **gridIcon** - Properties of the icons displayed on the bottom screen in grid display modes.
- **bannerListIcon** - Properties of the icons displayed on the bottom screen in banner list display mode.
- **bannerListTextLine0** - Properties of the first banner text line displayed on the bottom screen in banner list display mode.
- **bannerListTextLine1** - Properties of the second banner text line displayed on the bottom screen in banner list display mode.
- **bannerListTextLine2** - Properties of the third banner text line displayed on the bottom screen in banner list display mode.

Blend colors are used to fake translucency. They should be set to an approximation of the background color.

```json
{
    "type": "custom",
    "name": "Raspberry",
    "description": "Theme based on raspberries.",
    "author": "Gericom",
    "primaryColor": { "r": 138, "g": 217, "b": 255 },
    "darkTheme": false,
    "topIcon": {
        "position": { "x": 24, "y": 132 },
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "topBannerTextLine0": {
        "position": { "x": 70, "y": 126 },
        "width": 168,
        "textColor": { "r": 30, "g": 30, "b": 30 },
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "topBannerTextLine1": {
        "position": { "x": 70, "y": 141 },
        "width": 168,
        "textColor": { "r": 30, "g": 30, "b": 30 },
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "topBannerTextLine2": {
        "position": { "x": 70, "y": 155 },
        "width": 168,
        "textColor": { "r": 30, "g": 30, "b": 30 },
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "topFileNameText": {
        "position": { "x": 18, "y": 170 },
        "width": 220,
        "textColor": { "r": 30, "g": 30, "b": 30 },
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "topCover": {
        "position": { "x": 75, "y": 18 }
    },
    "topGameCount": {
        "position": { "x": 4, "y": 2 }
    },
    "topLaunchInfo": {
        "position": { "x": 252, "y": 2 },
        "hidden": false
    },
    "gridIcon": {
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "bannerListIcon": {
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "bannerListTextLine0": {
        "textColor": { "r": 30, "g": 30, "b": 30 }
    },
    "bannerListTextLine1": {
        "textColor": { "r": 30, "g": 30, "b": 30 }
    },
    "bannerListTextLine2": {
        "textColor": { "r": 30, "g": 30, "b": 30 }
    }
}
```

## DS BIOS type
![DS BIOS theme in coverflow mode](images/DsBios.png)

The `dsbios` type draws the Nintendo DS home screen: an analog clock, a month calendar, a status bar carrying the console's user name, the time and the date, and a game title box holding the selected game's icon and banner.

Everything is drawn in code at DS pixel scale from the theme's palette, so a `dsbios` theme ships **no bitmaps at all** — a `theme.json` is the entire theme. Use the `colors` block above to pin the palette; the defaults derive from `primaryColor` like any other type.

The clock hands and the calendar come from the console's real-time clock, and the user name from the console's own system settings. The calendar grid is seven columns by five rows, as on the original; a month that needs a sixth week gives up its first row once the current day passes the fifth, so today is always on screen.

In grid display modes the cover art replaces the clock panel entirely, as it does on Pico Launcher's other themes:

![DS BIOS theme in a grid display mode](images/DsBiosGrid.png)

Because the layout is drawn rather than painted, it is a theme and not a fork: favorites, play stats, recently played, cheats and guides all keep working underneath it.

The look is a rebuild of [Lohkas](https://www.reddit.com/r/flashcarts/)'s DS BIOS theme, which achieved it by hardcoding a modified launcher build rather than as a theme.

## Background music
All themes support background music by placing DSP-ADPCM encoded `.bcstm` files in a `bgm` folder inside the theme folder. Looping is supported. When multiple `.bcstm` files are provided, the background music will be selected at random each time Pico Launcher is started.

## Theme selector icon
A theme can have an `icon.bmp` file that is shown in the theme list when selecting a theme. It must be **32×32 pixels, 4 bpp (16 colors), uncompressed `.bmp`** file, with the first palette color treated as transparent.

## Theme selector preview image
A theme can have a `preview.bin` file that is shown on the top screen in the theme selection screen. It must be a 256x192 pixels 15 bpp bitmap (same format as `topbg.bin` and `bottombg.bin`). When it is not provided, `topbg.bin` is displayed instead. If that does not exist either, nothing is shown on the top screen.
