#include "common.h"
#include <libtwl/mem/memVram.h>
#include "romBrowser/Theme/Material/CarouselRecyclerView.h"
#include "DsBiosTheme.h"

// Everything the file info view draws, placed inside the game title box the BIOS
// puts under the clock and calendar: x15..239 y149..186, measured off Lohkas's
// build. The icon sits at the left, three banner lines run beside it at an 11px
// pitch - tighter than his two-line spacing, because a DS banner title can carry
// three lines and dropping one loses part of the name - and the right end is
// left clear for the play-stats pill.
// The filename and the elements this theme does not use are parked below the
// screen, where LabelView::GetBounds puts them outside the visible rectangle and
// Draw skips them.
#define BOX_TEXT_X      59
#define BOX_TEXT_W      138
#define BOX_LINE0_Y     152
#define BOX_LINE1_Y     163
#define BOX_LINE2_Y     174
#define OFFSCREEN_Y     200

CustomThemeInfo DsBiosTheme::MakeGameBoxLayout(const MaterialColorScheme& scheme)
{
    const Rgb8& ink = scheme.onSurface;
    const Rgb8& muted = scheme.onSurfaceVariant;
    const Rgb8& panel = scheme.surfaceBright;
    return CustomThemeInfo
    {
        .topIconInfo = CustomTopIconInfo(Point(19, 152), panel),
        .topBannerTextLine0Info = CustomTopTextElementInfo(Point(BOX_TEXT_X, BOX_LINE0_Y), BOX_TEXT_W, ink, panel),
        .topBannerTextLine1Info = CustomTopTextElementInfo(Point(BOX_TEXT_X, BOX_LINE1_Y), BOX_TEXT_W, muted, panel),
        .topBannerTextLine2Info = CustomTopTextElementInfo(Point(BOX_TEXT_X, BOX_LINE2_Y), BOX_TEXT_W, muted, panel),
        .topFileNameTextInfo = CustomTopTextElementInfo(Point(0, OFFSCREEN_Y), 220, muted, panel),
        .topCoverInfo = CustomTopCoverInfo(Point(15, 47)),
        .topGameCountInfo = CustomTopStripElementInfo(Point(4, 2), true),
        .topLaunchInfoInfo = CustomTopStripElementInfo(Point(235, 160), false),

        .gridIconInfo = CustomBottomIconInfo(panel),

        .bannerListIconInfo = CustomBottomIconInfo(panel),
        .bannerListTextLine0Info = CustomBannerListTextElementInfo(ink),
        .bannerListTextLine1Info = CustomBannerListTextElementInfo(muted),
        .bannerListTextLine2Info = CustomBannerListTextElementInfo(muted)
    };
}

// The view factory is Material's, so its carousel needs the same graphics
// uploaded that MaterialTheme uploads for it.
void DsBiosTheme::LoadRomBrowserResources(const VramContext& mainVramContext, const VramContext& subVramContext)
{
    mem_setVramDMapping(MEM_VRAM_D_LCDC);
    mem_setVramEMapping(MEM_VRAM_E_LCDC);
    CarouselRecyclerView::UploadGraphics(mainVramContext);
    mem_setVramDMapping(MEM_VRAM_D_TEX_SLOT_0);
    mem_setVramEMapping(MEM_VRAM_E_TEX_PLTT_SLOT_0123);
}
