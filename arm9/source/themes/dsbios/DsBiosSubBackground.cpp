#include "common.h"
#include <algorithm>
#include <string.h>
#include <nds/arm9/background.h>
#include <nds/system.h>
#include <nds/arm9/trig_lut.h>
#include "core/mini-printf.h"
#include "core/StringUtil.h"
#include "gui/GraphicsContext.h"
#include "gui/VramContext.h"
#include "rtcIpc.h"
#include "../ITheme.h"
#include "DsBiosCalendar.h"
#include "DsBiosSubBackground.h"

// Status strip geometry, measured off a true scale capture of the DS Lite home
// screen itself (Downloads/DS_Menu.webp, 256x384): the strip is y0..14 with a
// ONE pixel rule at y15. build-theme.py drew that rule 2px tall, which is a
// drift from the original, not the original.
#define STRIP_BOTTOM    15
#define RULE_BOTTOM     16

// Medium10, the face the rest of the launcher uses. The 7.5 one is closer to the
// original's 5x7 bitmap width but renders washed out at this size - too few solid
// pixels survive the 4bpp antialiasing - so the dividers move instead of the text.
#define LABEL_FONT      FontType::Medium10
#define LABEL_H         16
#define LABEL_Y         1

// The original packs its clock and date at x148..202 and fills x206..252 with a
// mode, cartridge and battery icon. We have none of those, so the cluster is
// right aligned to the screen instead of holding empty space for them: every
// anchor below is the original's plus 50. Internal spacing is untouched, so if
// those icons ever arrive the whole group shifts back by subtracting 50.
#define NAME_X          3
#define NAME_W          96
// The clock is anchored on its colon rather than its right edge, because the
// hour is end aligned onto it and the minute starts just after it, which is what
// pins both digit pairs while the colon blinks.
#define COLON_X         200
#define MINUTE_X        204
#define DATE_RIGHT      252
// Medium10 sets a 5 character group ~28px wide against the original bitmap
// font's 23, so the cluster cannot hold every one of the original's x. The date's
// right edge at 202 is the one that matters - it is what the icons butt against -
// so that stays and the clock and dividers slide left to keep 4px of air on
// either side of each rule.
#define DIV_NAME_X      185
#define DIV_TIME_X      218

// Calendar, measured off the original: the panel frame is x127..239 y47..143,
// the weekday header band ends at y63, and every cell is a strict 16px square,
// so column edges are x127+16k and week rows y63+16r. The month header sits
// ABOVE the panel, centred on it, ink at y35..43.
#define CAL_X0          127
#define CAL_X1          239
#define CAL_Y0          47
#define CAL_Y1          143
#define CAL_HDR_BOTTOM  63
#define CELL            16
#define MONTH_Y         32

// Sunday red and Saturday blue are the BIOS calendar's fixed meaning rather than
// a theme colour, so they are constants here and not roles off the seed. A theme
// that wants them can still override the roles they sit on via theme.json.
// Clock, measured off the original: panel x15..111 y47..143 - the same y band as
// the calendar - centred on (63,95) with a 5x5 dot there. The eight 4x4 ticks are
// listed rather than derived, because their radius rounds asymmetrically and the
// listed corners are what the original actually uses. Hands: the hour is shorter
// and fatter than the minute, exactly as on a watch, which is what tells them
// apart without needing two colours.
#define CLK_X0          15
#define CLK_X1          111
#define CLK_Y0          47
// one row shorter than the calendar, so the 96 row cover hides the panel exactly.
// The two panels share a top edge, which is the alignment that reads.
#define CLK_Y1          142
#define CLK_CX          63
#define CLK_CY          94
#define CLK_DOT         2
#define HOUR_LEN        31
#define HOUR_W          3
#define MINUTE_LEN      36
#define MINUTE_W        2

// Game title box, measured off Lohkas's build. It spans the full width the clock
// and calendar cover between them, so its edges line up with theirs.
#define BOX_X0          15
#define BOX_X1          239
#define BOX_Y0          149
#define BOX_Y1          186

// Sampled off the original rather than chosen: its weekend columns are far more
// saturated than a tasteful pale tint, and the header cells are SOLID chips with
// white caps on them, which is most of what makes the grid read as the DS one.
static const Rgb8 kSundayTint(251, 162, 235);
static const Rgb8 kSundayInk(121, 0, 0);
static const Rgb8 kSundayChip(211, 0, 0);
static const Rgb8 kSaturdayTint(130, 170, 251);
static const Rgb8 kSaturdayInk(0, 0, 130);
static const Rgb8 kSaturdayChip(0, 65, 195);
static const Rgb8 kChipInk(255, 255, 255);
// wide enough that a 5 character group never gets a glyph clipped off it
#define CLOCK_W         40

// the sub screen's 16bpp bitmap background, same slot the custom theme's
// topbg.bin lands in (BG_MAP_BASE(2) on an extended affine bg = 16KB units)
static u16* const sBitmap = (u16*)((u8*)BG_GFX_SUB + 0x8000);

// A1BGR5551: bit15 is the opaque flag, and a pixel written without it renders
// transparent rather than black.
static u16 toBitmapColor(const Rgb<8, 8, 8>& color)
{
    return 0x8000 | (color.r >> 3) | ((color.g >> 3) << 5) | ((color.b >> 3) << 10);
}

// The BIOS page is squared paper, not a flat fill: a light hatch on every odd
// row, and a darker rule every 16th row and column. All three tones are derived
// from the palette so the pattern survives any seed.
#define GRID_PITCH      16

static void fillRows(u16*& dst, int rowCount, u16 color)
{
    for (int i = 0; i < rowCount * 256; i++)
        *dst++ = color;
}

static Rgb8 lerpColor(const Rgb8& a, const Rgb8& b, int num, int den)
{
    return Rgb8(a.r + (b.r - a.r) * num / den,
                a.g + (b.g - a.g) * num / den,
                a.b + (b.b - a.b) * num / den);
}

static void fillRect(int x0, int y0, int x1, int y1, u16 color)
{
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > 255) x1 = 255;
    if (y1 > 191) y1 = 191;
    for (int y = y0; y <= y1; y++)
    {
        u16* row = sBitmap + y * 256;
        for (int x = x0; x <= x1; x++)
            row[x] = color;
    }
}

// The original's segment dividers are dotted two rows on, one row off, phased so
// the run starts inked at y0.
static void drawDivider(int x, u16 color)
{
    for (int y = 0; y < STRIP_BOTTOM; y++)
    {
        if (y % 3 != 2)
            sBitmap[y * 256 + x] = color;
    }
}

void DsBiosSubBackground::PaintBackground() const
{
    // inverseOnSurface is the background role, not surfaceBright - getting that
    // mapping backwards has cost a wrong build before.
    const Rgb8& page = _materialColorScheme->inverseOnSurface;
    const Rgb8& ink = _materialColorScheme->outline;
    // the strip is its own pair of roles rather than the panel's, because the
    // BIOS bar is a dark blue-grey with light text while the panels under it are
    // light with dark text - one pair cannot be both
    const Rgb8& strip = _materialColorScheme->secondaryContainer;
    const Rgb8& stripInk = _materialColorScheme->onSecondaryContainer;

    u16 pageColor = toBitmapColor(page);
    u16 hatchColor = toBitmapColor(lerpColor(page, ink, 1, 8));
    u16 gridColor = toBitmapColor(lerpColor(page, ink, 1, 4));
    for (int y = RULE_BOTTOM; y < 192; y++)
    {
        u16* row = sBitmap + y * 256;
        u16 base = (y % GRID_PITCH) == GRID_PITCH - 1 ? gridColor
            : ((y & 1) ? hatchColor : pageColor);
        for (int x = 0; x < 256; x++)
            row[x] = (x % GRID_PITCH) == GRID_PITCH - 1 ? gridColor : base;
    }

}

// The status bar is painted into the bitmap like everything else in this theme,
// NOT built from OAM labels. It used to be five Label2DViews, which cost 2560
// bytes of sub OBJ vram - and the guide reader allocates 9 x 1792 = 16128 of the
// 16384 that VRAM_I holds, sized on the assumption that a theme background uses
// none. AscendingStackVramManager::Alloc has NO bounds check, so those 2560
// bytes pushed the reader's labels over the end, they wrapped onto the start of
// the bank, and the status bar's tiles were overwritten - the user name never
// came back because the background is not recreated on the way out. Painting the
// bar costs zero obj vram and cannot be clobbered.
void DsBiosSubBackground::PaintStatusBar()
{
    const Rgb8& strip = _materialColorScheme->secondaryContainer;
    const Rgb8& stripInk = _materialColorScheme->onSecondaryContainer;
    const Rgb8& ink = _materialColorScheme->outline;

    // lighter at the top, which is most of what makes it read as the BIOS bar
    Rgb8 stripTop = lerpColor(strip, stripInk, 1, 4);
    u16* dst = sBitmap;
    for (int y = 0; y < STRIP_BOTTOM; y++)
        fillRows(dst, 1, toBitmapColor(lerpColor(stripTop, strip, y, STRIP_BOTTOM - 1)));
    fillRows(dst, RULE_BOTTOM - STRIP_BOTTOM, toBitmapColor(ink));

    u16 dividerColor = toBitmapColor(lerpColor(strip, ink, 1, 2));
    drawDivider(DIV_NAME_X, dividerColor);
    drawDivider(DIV_TIME_X, dividerColor);

    // the glyphs blend towards the middle of the gradient they sit on
    Rgb8 behind = lerpColor(stripTop, strip, STRIP_BOTTOM / 2, STRIP_BOTTOM - 1);
    if (_userName[0])
        DrawText16(NAME_X, LABEL_Y, _userName, stripInk, behind, Anchor::Left);

    char pair[8];
    mini_snprintf(pair, sizeof(pair), "%02x", _bcdHour);
    DrawText(COLON_X, LABEL_Y, pair, stripInk, behind, 1, Anchor::Right);
    mini_snprintf(pair, sizeof(pair), "%02x", _bcdMinute);
    DrawText(MINUTE_X, LABEL_Y, pair, stripInk, behind, 1, Anchor::Left);
    if (_colonLit > 0)
        DrawText(COLON_X + 1, LABEL_Y, ":", stripInk, behind, 1, Anchor::Left);

    DrawText(DATE_RIGHT, LABEL_Y, _dateText, stripInk, behind, 1, Anchor::Right);
}

// Hands radiate from the clock centre. `turns` is the position as a fraction
// numerator over `perTurn`, so the minute hand passes 60 and the hour hand 720,
// and both land on the same code.
static void drawHand(int turns, int perTurn, int length, int width, u16 color)
{
    s16 angle = (s16)((turns * DEGREES_IN_CIRCLE) / perTurn);
    int dx = sinLerp(angle);
    int dy = -cosLerp(angle);
    int half = width / 2;
    for (int t = 0; t <= length; t++)
    {
        int x = CLK_CX + (dx * t) / 4096;
        int y = CLK_CY + (dy * t) / 4096;
        fillRect(x - half, y - half, x - half + width - 1, y - half + width - 1, color);
    }
}

// Draws one short string straight into the screen bitmap, centred on centerX.
// nft2's a5i3 path is the one to use here: it writes one linear byte per pixel
// whose top nibble is the glyph's coverage, which is exactly what a 16 step
// colour ramp wants. Its other path writes OBJ tiles, which would have to be
// de-swizzled first. Text only ever lands on flat panel fill, so the ramp can be
// built once per call from the fill colour rather than read back per pixel.
void DsBiosSubBackground::DrawText(int x, int y, const char* text,
    const Rgb8& fg, const Rgb8& bg, int scale, Anchor anchor)
{
    char16_t buffer[24];
    StringUtil::Copy(buffer, text, sizeof(buffer) / sizeof(buffer[0]));
    DrawTextImpl(x, y, buffer, fg, bg, scale, anchor);
}

void DsBiosSubBackground::DrawText16(int x, int y, const char16_t* text,
    const Rgb8& fg, const Rgb8& bg, Anchor anchor)
{
    DrawTextImpl(x, y, text, fg, bg, 1, anchor);
}

void DsBiosSubBackground::DrawTextImpl(int x, int y, const char16_t* buffer,
    const Rgb8& fg, const Rgb8& bg, int scale, Anchor anchor)
{
    memset(_textScratch, 0, sizeof(_textScratch));
    nft2_string_render_params_t params;
    params.x = 0;
    params.y = 0;
    params.width = kScratchW;
    params.height = kScratchH;
    params.textWidth = 0;
    params.a5i3 = true;
    params.onlyRenderWholeGlyphs = true;
    nft2_renderString(_font, buffer, _textScratch, kScratchW, &params);

    // Weighted towards the ink rather than linear. A linear ramp leaves thin
    // strokes sitting at half coverage, which renders the BIOS font's one pixel
    // stems as a wash - the original's numerals are near solid at this size.
    u16 ramp[16];
    for (int i = 0; i < 16; i++)
    {
        int t = 15 - ((15 - i) * (15 - i)) / 15;
        ramp[i] = toBitmapColor(lerpColor(bg, fg, t, 15));
    }

    int width = (int)params.textWidth;
    if (width > kScratchW)
        width = kScratchW;
    // scale is nearest neighbour pixel doubling, which is how the clock numerals
    // reach the size the original draws them at without a second font: blocky is
    // the BIOS look, and the alternative is hand rolling a big bitmap face.
    int drawn = width * scale;
    int x0 = anchor == Anchor::Left ? x
        : (anchor == Anchor::Right ? x - drawn : x - drawn / 2);
    for (int sy = 0; sy < kScratchH; sy++)
    {
        const u8* src = &_textScratch[sy * kScratchW];
        for (int sx = 0; sx < width; sx++)
        {
            u32 coverage = src[sx] >> 4;
            if (!coverage)
                continue;
            u16 color = ramp[coverage];
            for (int ry = 0; ry < scale; ry++)
            {
                int dy = y + sy * scale + ry;
                if (dy < 0 || dy > 191)
                    continue;
                u16* dst = sBitmap + dy * 256;
                for (int rx = 0; rx < scale; rx++)
                {
                    int dx = x0 + sx * scale + rx;
                    if (dx >= 0 && dx <= 255)
                        dst[dx] = color;
                }
            }
        }
    }
}

void DsBiosSubBackground::PaintGameBox() const
{
    u16 frameColor = toBitmapColor(_materialColorScheme->outline);
    fillRect(BOX_X0, BOX_Y0, BOX_X1, BOX_Y1, toBitmapColor(_materialColorScheme->surfaceBright));
    fillRect(BOX_X0, BOX_Y0, BOX_X1, BOX_Y0, frameColor);
    fillRect(BOX_X0, BOX_Y1, BOX_X1, BOX_Y1, frameColor);
    fillRect(BOX_X0, BOX_Y0, BOX_X0, BOX_Y1, frameColor);
    fillRect(BOX_X1, BOX_Y0, BOX_X1, BOX_Y1, frameColor);
}

void DsBiosSubBackground::PaintClock(int hour, int minute)
{
    // the original's tick corners, pulled up one row with the centre
    static const struct { u8 x, y; } sTicks[8] =
    {
        { 40, 55 }, { 84, 55 }, { 24, 71 }, { 100, 71 },
        { 24, 115 }, { 100, 115 }, { 40, 131 }, { 84, 131 }
    };
    // numeral centres, measured off the original; they sit inside the tick ring
    static const struct { const char* text; u8 x, y; } sNumerals[4] =
    {
        { "12", 63, 62 }, { "3", 95, 93 }, { "6", 63, 124 }, { "9", 30, 93 }
    };

    const Rgb8& panel = _materialColorScheme->surfaceBright;
    const Rgb8& ink = _materialColorScheme->onSurface;
    const Rgb8& frame = _materialColorScheme->outline;
    // the original keeps a hierarchy that matters: numerals lighter than the
    // ticks, ticks lighter than the hands, so the hands always read first
    Rgb8 numeralInk = lerpColor(panel, ink, 2, 5);
    Rgb8 tickInk = lerpColor(panel, ink, 1, 2);

    fillRect(CLK_X0, CLK_Y0, CLK_X1, CLK_Y1, toBitmapColor(panel));
    u16 frameColor = toBitmapColor(frame);
    fillRect(CLK_X0, CLK_Y0, CLK_X1, CLK_Y0, frameColor);
    fillRect(CLK_X0, CLK_Y1, CLK_X1, CLK_Y1, frameColor);
    fillRect(CLK_X0, CLK_Y0, CLK_X0, CLK_Y1, frameColor);
    fillRect(CLK_X1, CLK_Y0, CLK_X1, CLK_Y1, frameColor);

    for (int i = 0; i < 4; i++)
    {
        // the 10px face doubles to roughly the 20px the original's numerals run
        DrawText(sNumerals[i].x, sNumerals[i].y - 10, sNumerals[i].text,
            numeralInk, panel, 2);
    }

    u16 tickColor = toBitmapColor(tickInk);
    for (int i = 0; i < 8; i++)
        fillRect(sTicks[i].x, sTicks[i].y, sTicks[i].x + 3, sTicks[i].y + 3, tickColor);

    u16 handColor = toBitmapColor(ink);
    drawHand((hour % 12) * 60 + minute, 720, HOUR_LEN, HOUR_W, handColor);
    drawHand(minute, 60, MINUTE_LEN, MINUTE_W, handColor);
    fillRect(CLK_CX - CLK_DOT, CLK_CY - CLK_DOT, CLK_CX + CLK_DOT, CLK_CY + CLK_DOT, handColor);
}

void DsBiosSubBackground::PaintCalendar(int year, int month, int day)
{
    static const char* const sWeekdays[7] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" };

    const Rgb8& panel = _materialColorScheme->surfaceBright;
    const Rgb8& ink = _materialColorScheme->onSurface;
    const Rgb8& frame = _materialColorScheme->outline;
    const Rgb8& page = _materialColorScheme->inverseOnSurface;
    // inner cell rules read as a lighter version of the frame in any palette,
    // which is what keeps the grid from looking like a table of boxes
    Rgb8 rule = lerpColor(panel, frame, 1, 2);
    Rgb8 todayTint = lerpColor(panel, _materialColorScheme->primary, 1, 5);

    u16 panelColor = toBitmapColor(panel);
    u16 frameColor = toBitmapColor(frame);
    u16 ruleColor = toBitmapColor(rule);

    // clear the band the month header lives in, so a repaint at midnight does
    // not leave the old month behind it
    fillRect(CAL_X0, MONTH_Y, CAL_X1, CAL_Y0 - 1, toBitmapColor(page));

    fillRect(CAL_X0, CAL_Y0, CAL_X1, CAL_Y1, panelColor);

    int firstColumn = DsBiosCalendar::ColumnOfFirst(year, month);
    int dayCount = DsBiosCalendar::DaysInMonth(year, month);
    int firstWeek = DsBiosCalendar::FirstVisibleWeek(year, month, day);

    // weekend tints run the body only; the header cells get solid chips below
    fillRect(CAL_X0 + 1, CAL_HDR_BOTTOM, CAL_X0 + CELL - 1, CAL_Y1 - 1, toBitmapColor(kSundayTint));
    fillRect(CAL_X1 - CELL + 1, CAL_HDR_BOTTOM, CAL_X1 - 1, CAL_Y1 - 1, toBitmapColor(kSaturdayTint));

    // grid: light rules inside, the frame and the header underline in ink
    for (int r = 1; r < DsBiosCalendar::kRows; r++)
    {
        int y = CAL_HDR_BOTTOM + r * CELL;
        fillRect(CAL_X0 + 1, y, CAL_X1 - 1, y, ruleColor);
    }
    for (int c = 1; c < DsBiosCalendar::kColumns; c++)
    {
        int x = CAL_X0 + c * CELL;
        fillRect(x, CAL_Y0 + 1, x, CAL_Y1 - 1, ruleColor);
        // the header band keeps the original's harder separators
        fillRect(x, CAL_Y0 + 1, x, CAL_HDR_BOTTOM - 2, frameColor);
    }
    fillRect(CAL_X0, CAL_Y0, CAL_X1, CAL_Y0, frameColor);
    fillRect(CAL_X0, CAL_Y1, CAL_X1, CAL_Y1, frameColor);
    fillRect(CAL_X0, CAL_Y0, CAL_X0, CAL_Y1, frameColor);
    fillRect(CAL_X1, CAL_Y0, CAL_X1, CAL_Y1, frameColor);
    fillRect(CAL_X0 + 1, CAL_HDR_BOTTOM - 2, CAL_X1 - 1, CAL_HDR_BOTTOM - 1, frameColor);

    // today's cell, drawn before the numbers so the number sits on top of it
    int todayWeek = (firstColumn + day - 1) / DsBiosCalendar::kColumns;
    int todayRow = todayWeek - firstWeek;
    if (todayRow >= 0 && todayRow < DsBiosCalendar::kRows)
    {
        int column = (firstColumn + day - 1) % DsBiosCalendar::kColumns;
        int cx = CAL_X0 + column * CELL;
        int cy = CAL_HDR_BOTTOM + todayRow * CELL;
        fillRect(cx + 1, cy + 1, cx + CELL - 1, cy + CELL - 1, toBitmapColor(todayTint));
        fillRect(cx + 1, cy + 1, cx + CELL - 1, cy + 1, toBitmapColor(_materialColorScheme->primary));
        fillRect(cx + 1, cy + CELL - 1, cx + CELL - 1, cy + CELL - 1, toBitmapColor(_materialColorScheme->primary));
        fillRect(cx + 1, cy + 1, cx + 1, cy + CELL - 1, toBitmapColor(_materialColorScheme->primary));
        fillRect(cx + CELL - 1, cy + 1, cx + CELL - 1, cy + CELL - 1, toBitmapColor(_materialColorScheme->primary));
    }

    char text[8];
    mini_snprintf(text, sizeof(text), "%02d/%04d", month, year);
    DrawText((CAL_X0 + CAL_X1) / 2, MONTH_Y, text, ink, page);

    // EVERY header cell is a filled chip with a white cap, not just the weekend
    // ones - the midweek chips are grey, which is easy to mistake for the caps'
    // antialiasing when measuring and is a big part of the grid's weight.
    Rgb8 weekdayChip = lerpColor(panel, ink, 3, 5);
    for (int c = 0; c < DsBiosCalendar::kColumns; c++)
    {
        const Rgb8& chip = c == 0 ? kSundayChip : (c == 6 ? kSaturdayChip : weekdayChip);
        int cx = CAL_X0 + c * CELL;
        fillRect(cx + 1, CAL_Y0 + 1, cx + CELL - 1, CAL_HDR_BOTTOM - 3, toBitmapColor(chip));
        DrawText(cx + CELL / 2, CAL_Y0 + 1, sWeekdays[c], kChipInk, chip);
    }

    for (int d = 1; d <= dayCount; d++)
    {
        int week = (firstColumn + d - 1) / DsBiosCalendar::kColumns;
        int row = week - firstWeek;
        if (row < 0 || row >= DsBiosCalendar::kRows)
            continue;
        int column = (firstColumn + d - 1) % DsBiosCalendar::kColumns;
        const Rgb8& dayInk = column == 0 ? kSundayInk : (column == 6 ? kSaturdayInk : ink);
        Rgb8 dayBg = column == 0 ? kSundayTint : (column == 6 ? kSaturdayTint : panel);
        if (d == day)
            dayBg = todayTint;
        mini_snprintf(text, sizeof(text), "%d", d);
        DrawText(CAL_X0 + column * CELL + CELL / 2,
            CAL_HDR_BOTTOM + row * CELL + 1, text, dayInk, dayBg);
    }
}

// The name the owner typed into their DS's system settings, which is what the
// BIOS itself shows here. The firmware copies it to PersonalData at boot; a card
// whose firmware never ran leaves nameLen at 0, and then the slot stays empty
// rather than showing a placeholder that isn't the user's.
void DsBiosSubBackground::ReadUserName()
{
    // PERSONAL_DATA is a packed struct, so copy the name out rather than
    // handing a pointer into it
    u32 nameLen = std::min<u32>(PersonalData->nameLen, 10);
    memcpy(_userName, (const void*)PersonalData->name, nameLen * sizeof(char16_t));
    _userName[nameLen] = 0;
}

void DsBiosSubBackground::UpdateDateTime()
{
    rtc_datetime_t dateTime;
    rtc_readDateTime(&dateTime);

    // the rtc registers hold BCD values, which %x renders as decimal digits
    char timeText[sizeof(_timeText)];
    mini_snprintf(timeText, sizeof(timeText), "%02x%02x",
        dateTime.time.hour, dateTime.time.minute);

    // the colon blinks once a second, as the BIOS clock's does. Lit on even
    // seconds, so a stopped clock reading zero shows it rather than hiding it.
    u32 second = (dateTime.time.second >> 4) * 10 + (dateTime.time.second & 0xF);
    int colonLit = (second & 1) ? 0 : 1;
    bool barDirty = colonLit != _colonLit;
    _colonLit = colonLit;

    // day/month, the numeric pair the BIOS shows here
    char dateText[sizeof(_dateText)];
    mini_snprintf(dateText, sizeof(dateText), "%02x/%02x",
        dateTime.date.monthDay, dateTime.date.month);

    // BCD to plain numbers for the calendar's arithmetic. A clock that was never
    // set reads month and day as zero, which is not a date any calendar can be
    // drawn from, so it falls back to the 1st exactly as the real BIOS does when
    // it shows 01/2000 on a fresh console.
    int year = 2000 + (dateTime.date.year >> 4) * 10 + (dateTime.date.year & 0xF);
    int month = (dateTime.date.month >> 4) * 10 + (dateTime.date.month & 0xF);
    int monthDay = (dateTime.date.monthDay >> 4) * 10 + (dateTime.date.monthDay & 0xF);
    if (month < 1 || month > 12)
        month = 1;
    if (monthDay < 1 || monthDay > DsBiosCalendar::DaysInMonth(year, month))
        monthDay = 1;
    if (year != _paintedYear || month != _paintedMonth || monthDay != _paintedDay)
    {
        // outside vblank, so a repaint can tear for one frame - which happens at
        // boot and again at midnight, and costs less than holding a second copy
        // of the screen to page flip from
        PaintCalendar(year, month, monthDay);
        _paintedYear = year;
        _paintedMonth = month;
        _paintedDay = monthDay;
    }

    // both only change once a minute, so re-render only on a real change and the
    // strip costs one rtc read a second and nothing else
    if (strcmp(timeText, _timeText) != 0)
    {
        strcpy(_timeText, timeText);
        _bcdHour = dateTime.time.hour;
        _bcdMinute = dateTime.time.minute;
        barDirty = true;
        // the hands only move on the minute, so the face is repainted here
        // rather than every second
        PaintClock((dateTime.time.hour >> 4) * 10 + (dateTime.time.hour & 0xF),
            (dateTime.time.minute >> 4) * 10 + (dateTime.time.minute & 0xF));
    }
    if (strcmp(dateText, _dateText) != 0)
    {
        strcpy(_dateText, dateText);
        barDirty = true;
    }

    if (barDirty)
        PaintStatusBar();
}

void DsBiosSubBackground::LoadResources(const ITheme& theme, const VramContext& vramContext)
{
    PaintBackground();
    PaintGameBox();

    _font = theme.GetFontRepository()->GetFont(LABEL_FONT);
    ReadUserName();
    UpdateDateTime();

}

void DsBiosSubBackground::Update()
{
    if (--_rtcCountdown <= 0)
    {
        _rtcCountdown = 60; // Update() runs once a frame
        UpdateDateTime();
    }
}

void DsBiosSubBackground::VBlank()
{
    REG_DISPCNT_SUB = (REG_DISPCNT_SUB & ~0xF) | 5 | (4 << 8);
    REG_BG2CNT_SUB = BG_BMP16_256x256 | BG_PRIORITY_3 | BG_COLOR_16 | BG_MAP_BASE(2);
    REG_BG2HOFS_SUB = 0;
    REG_BG2VOFS_SUB = 0;
    REG_BG2X_SUB = 0;
    REG_BG2Y_SUB = 0;
    REG_BG2PA_SUB = 256;
    REG_BG2PB_SUB = 0;
    REG_BG2PC_SUB = 0;
    REG_BG2PD_SUB = 256;

}
