#pragma once
#include "core/math/Rgb.h"
#include "gui/font/nitroFont2.h"
#include "../background/IThemeBackground.h"
#include "../material/MaterialColorScheme.h"

/// @brief Top screen of the DS BIOS theme. Everything on it is drawn in code at
///        DS pixel scale - the strip and its rule straight into the sub screen's
///        16bpp bitmap, the text as label OBJs - because bitmap art at this pixel
///        density is what made the old rml theme misalign.
class DsBiosSubBackground : public IThemeBackground
{
public:
    explicit DsBiosSubBackground(const MaterialColorScheme* materialColorScheme)
        : _materialColorScheme(materialColorScheme) { }

    void LoadResources(const ITheme& theme, const VramContext& vramContext) override;
    void Update() override;
    void VBlank() override;

private:
    const MaterialColorScheme* _materialColorScheme;
    // Update() runs once a frame; the rtc is only read when this hits zero.
    int _rtcCountdown = 0;
    char _timeText[8] = {};
    int _colonLit = -1;
    char _dateText[16] = {};
    // the calendar is repainted only when the date it shows actually changes,
    // which in practice is once at boot and once at midnight
    int _paintedYear = 0;
    int _paintedMonth = 0;
    int _paintedDay = 0;
    const nft2_header_t* _font = nullptr;
    // glyph scratch for the bitmap text blitter; one linear byte per pixel
    static constexpr int kScratchW = 64;
    static constexpr int kScratchH = 16;
    u8 _textScratch[kScratchW * kScratchH];

    // Anchors for the bitmap text blitter.
    enum class Anchor { Left, Centre, Right };

    void PaintBackground() const;
    void PaintStatusBar();
    void ReadUserName();
    void PaintCalendar(int year, int month, int day);
    void PaintClock(int hour, int minute);
    void PaintGameBox() const;
    void DrawText(int x, int y, const char* text, const Rgb8& fg, const Rgb8& bg,
        int scale = 1, Anchor anchor = Anchor::Centre);
    void DrawText16(int x, int y, const char16_t* text, const Rgb8& fg, const Rgb8& bg,
        Anchor anchor);
    void DrawTextImpl(int x, int y, const char16_t* text, const Rgb8& fg, const Rgb8& bg,
        int scale, Anchor anchor);
    char16_t _userName[12] = {};
    u8 _bcdHour = 0;
    u8 _bcdMinute = 0;
    void UpdateDateTime();
};
