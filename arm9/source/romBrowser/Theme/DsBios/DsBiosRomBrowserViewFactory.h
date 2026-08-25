#pragma once
#include "../Material/MaterialRomBrowserViewFactory.h"
#include "../custom/CustomFileInfoView.h"
#include "themes/custom/CustomThemeInfo.h"

// The BIOS bottom screen is close enough to Material's to be Material's, so this
// only moves the top screen's two pills off the status strip, which the clock,
// user name and date now own:
//   - the game count goes entirely; the BIOS has no such readout and the strip
//     is full. The L/R sort-letter flash rides on that chip, so it goes with it.
//   - the launch info pill (play stats + favourite heart) drops onto the game
//     title box, where it reads as the "now playing" badge of the mockups.
// The grid-mode cover is placed to swallow the clock panel WHOLE - frame and all -
// rather than being clipped inside its border the way Lohkas does it. The shared
// top screen windows the cover over [x, x+106) by [y, y+96), so (15,47) covers
// exactly x15..120 y47..142; the clock panel is drawn to match that box, so no
// edge of it can peek out from behind the art.
class DsBiosRomBrowserViewFactory : public MaterialRomBrowserViewFactory
{
public:
    DsBiosRomBrowserViewFactory(const CustomThemeInfo* customThemeInfo,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository)
        : MaterialRomBrowserViewFactory(materialColorScheme, fontRepository)
        , _customThemeInfo(customThemeInfo), _fontRepository(fontRepository) { }

    // Material's card lays itself out across the middle of the top screen, which
    // is where the clock and calendar now are. CustomFileInfoView draws the same
    // icon and banner lines but takes every position from a CustomThemeInfo, so
    // it drops into the BIOS game title box with no new view to write and no
    // theme assets to ship.
    SharedPtr<BannerView> CreateFileInfoView() const override
    {
        return CustomFileInfoView::CreateShared(_customThemeInfo, _fontRepository);
    }

    Point GetTopCoverPosition() const override
    {
        return Point(15, 47);
    }

    TopStripElementLayout GetTopGameCountLayout() const override
    {
        return { Point(4, 2), true };
    }

    // right end of the game title box, 4px inside its frame
    TopStripElementLayout GetTopLaunchInfoLayout() const override
    {
        return { Point(235, 160), false };
    }

private:
    const CustomThemeInfo* _customThemeInfo;
    const IFontRepository* _fontRepository;
};
