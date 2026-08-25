#include "common.h"
#include <memory>
#include <libtwl/mem/memVram.h>
#include <libtwl/gfx/gfx.h>
#include <libtwl/gfx/gfxBackground.h>
#include "json/ArduinoJson.h"
#include "core/mini-printf.h"
#include "CustomTopBackgroundType.h"
#include "gui/VramContext.h"
#include "../material/MaterialColorSchemeFactory.h"
#include "romBrowser/views/IconButton3DView.h"
#include "../material/MaterialColorScheme.h"
#include "CustomTheme.h"

#define JSON_RESERVED_SIZE  4096

#define KEY_COLOR_R     "r"
#define KEY_COLOR_G     "g"
#define KEY_COLOR_B     "b"

#define KEY_POINT_X     "x"
#define KEY_POINT_Y     "y"

#define KEY_TOP_ICON                    "topIcon"
#define KEY_TOP_BANNER_TEXT_LINE_0      "topBannerTextLine0"
#define KEY_TOP_BANNER_TEXT_LINE_1      "topBannerTextLine1"
#define KEY_TOP_BANNER_TEXT_LINE_2      "topBannerTextLine2"
#define KEY_TOP_FILE_NAME_TEXT          "topFileNameText"
#define KEY_TOP_COVER                   "topCover"
#define KEY_TOP_GAME_COUNT              "topGameCount"
#define KEY_TOP_LAUNCH_INFO             "topLaunchInfo"
#define KEY_GRID_ICON                   "gridIcon"
#define KEY_BANNER_LIST_ICON            "bannerListIcon"
#define KEY_BANNER_LIST_TEXT_LINE_0     "bannerListTextLine0"
#define KEY_BANNER_LIST_TEXT_LINE_1     "bannerListTextLine1"
#define KEY_BANNER_LIST_TEXT_LINE_2     "bannerListTextLine2"

#define KEY_ELEMENT_POSITION        "position"
#define KEY_ELEMENT_WIDTH           "width"
#define KEY_ELEMENT_TEXT_COLOR      "textColor"
#define KEY_ELEMENT_BLEND_COLOR     "blendColor"
#define KEY_ELEMENT_HIDDEN          "hidden"

// Defaults come off the theme's own colour scheme, NOT from fixed near-black
// values. They used to be hardcoded Rgb8(30,30,30) text on Rgb8(200,200,200),
// which is invisible on any dark custom theme that does not override every text
// element - the wallpaper themes render their banner list as black on black.
// onSurface/inverseOnSurface give the right answer for light AND dark seeds.
static CustomThemeInfo makeDefaultCustomThemeInfo(const MaterialColorScheme& scheme)
{
    const Rgb8& ink = scheme.onSurface;
    const Rgb8& behind = scheme.inverseOnSurface;
    return CustomThemeInfo
    {
        .topIconInfo = CustomTopIconInfo(Point(24, 132), Rgb8(200, 200, 200)),
        .topBannerTextLine0Info = CustomTopTextElementInfo(Point(70, 126), 176, ink, behind),
        .topBannerTextLine1Info = CustomTopTextElementInfo(Point(70, 141), 176, ink, behind),
        .topBannerTextLine2Info = CustomTopTextElementInfo(Point(70, 155), 176, ink, behind),
        .topFileNameTextInfo = CustomTopTextElementInfo(Point(18, 170), 220, ink, behind),
        .topCoverInfo = CustomTopCoverInfo(Point(75, 18)),
        // top-left corner of the game count pill / top-right corner of the launch info pill
        .topGameCountInfo = CustomTopStripElementInfo(Point(4, 2), false),
        .topLaunchInfoInfo = CustomTopStripElementInfo(Point(252, 2), false),

        .gridIconInfo = CustomBottomIconInfo(Rgb8(200, 200, 200)),

        .bannerListIconInfo = CustomBottomIconInfo(Rgb8(200, 200, 200)),
        .bannerListTextLine0Info = CustomBannerListTextElementInfo(ink),
        .bannerListTextLine1Info = CustomBannerListTextElementInfo(ink),
        .bannerListTextLine2Info = CustomBannerListTextElementInfo(ink)
    };
}

static CustomTopBackgroundType parseTopBackgroundType(const char* topBackgroundTypeString)
{
    return CustomTopBackgroundType::Bitmap;
}

static Rgb8 parseColor(const JsonObjectConst& json, const Rgb8& defaultColor)
{
    if (json.isNull())
    {
        return defaultColor;
    }

    return Rgb8(
        json[KEY_COLOR_R] | 0,
        json[KEY_COLOR_G] | 0,
        json[KEY_COLOR_B] | 0
    );
}

static Point parsePoint(const JsonObjectConst& json, const Point& defaultPoint)
{
    if (json.isNull())
    {
        return defaultPoint;
    }

    return Point(
        json[KEY_POINT_X] | 0,
        json[KEY_POINT_Y] | 0
    );
}

static CustomBannerListTextElementInfo parseCustomBannerListTextElementInfo(
    const JsonObjectConst& json, const CustomBannerListTextElementInfo& defaultInfo)
{
    if (json.isNull())
    {
        return defaultInfo;
    }

    return CustomBannerListTextElementInfo(
        parseColor(json[KEY_ELEMENT_TEXT_COLOR], defaultInfo.GetTextColor())
    );
}

static CustomBottomIconInfo parseCustomBottomIconInfo(const JsonObjectConst& json, const CustomBottomIconInfo& defaultInfo)
{
    if (json.isNull())
    {
        return defaultInfo;
    }

    return CustomBottomIconInfo(
        parseColor(json[KEY_ELEMENT_BLEND_COLOR], defaultInfo.GetBlendColor())
    );
}

static CustomTopIconInfo parseCustomTopIconInfo(const JsonObjectConst& json, const CustomTopIconInfo& defaultInfo)
{
    if (json.isNull())
    {
        return defaultInfo;
    }

    return CustomTopIconInfo(
        parsePoint(json[KEY_ELEMENT_POSITION], defaultInfo.GetPosition()),
        parseColor(json[KEY_ELEMENT_BLEND_COLOR], defaultInfo.GetBlendColor())
    );
}

static CustomTopCoverInfo parseCustomTopCoverInfo(const JsonObjectConst& json, const CustomTopCoverInfo& defaultInfo)
{
    if (json.isNull())
    {
        return defaultInfo;
    }

    return CustomTopCoverInfo(
        parsePoint(json[KEY_ELEMENT_POSITION], defaultInfo.GetPosition())
    );
}

static CustomTopStripElementInfo parseCustomTopStripElementInfo(
    const JsonObjectConst& json, const CustomTopStripElementInfo& defaultInfo)
{
    if (json.isNull())
    {
        return defaultInfo;
    }

    return CustomTopStripElementInfo(
        parsePoint(json[KEY_ELEMENT_POSITION], defaultInfo.GetPosition()),
        json[KEY_ELEMENT_HIDDEN] | defaultInfo.GetIsHidden()
    );
}

static CustomTopTextElementInfo parseCustomTextElementInfo(
    const JsonObjectConst& json, const CustomTopTextElementInfo& defaultInfo)
{
    if (json.isNull())
    {
        return defaultInfo;
    }

    return CustomTopTextElementInfo(
        parsePoint(json[KEY_ELEMENT_POSITION], defaultInfo.GetPosition()),
        json[KEY_ELEMENT_WIDTH] | defaultInfo.GetWidth(),
        parseColor(json[KEY_ELEMENT_TEXT_COLOR], defaultInfo.GetTextColor()),
        parseColor(json[KEY_ELEMENT_BLEND_COLOR], defaultInfo.GetBlendColor())
    );
}

static CustomThemeInfo parseCustomThemeInfo(const JsonDocument& json, const CustomThemeInfo& defaults)
{
    return CustomThemeInfo
    {
        .topIconInfo = parseCustomTopIconInfo(json[KEY_TOP_ICON], defaults.topIconInfo),
        .topBannerTextLine0Info = parseCustomTextElementInfo(
            json[KEY_TOP_BANNER_TEXT_LINE_0], defaults.topBannerTextLine0Info),
        .topBannerTextLine1Info = parseCustomTextElementInfo(
            json[KEY_TOP_BANNER_TEXT_LINE_1], defaults.topBannerTextLine1Info),
        .topBannerTextLine2Info = parseCustomTextElementInfo(
            json[KEY_TOP_BANNER_TEXT_LINE_2], defaults.topBannerTextLine2Info),
        .topFileNameTextInfo = parseCustomTextElementInfo(
            json[KEY_TOP_FILE_NAME_TEXT], defaults.topFileNameTextInfo),
        .topCoverInfo = parseCustomTopCoverInfo(json[KEY_TOP_COVER], defaults.topCoverInfo),
        .topGameCountInfo = parseCustomTopStripElementInfo(
            json[KEY_TOP_GAME_COUNT], defaults.topGameCountInfo),
        .topLaunchInfoInfo = parseCustomTopStripElementInfo(
            json[KEY_TOP_LAUNCH_INFO], defaults.topLaunchInfoInfo),

        .gridIconInfo = parseCustomBottomIconInfo(json[KEY_GRID_ICON], defaults.gridIconInfo),

        .bannerListIconInfo = parseCustomBottomIconInfo(json[KEY_BANNER_LIST_ICON], defaults.bannerListIconInfo),
        .bannerListTextLine0Info = parseCustomBannerListTextElementInfo(
            json[KEY_BANNER_LIST_TEXT_LINE_0], defaults.bannerListTextLine0Info),
        .bannerListTextLine1Info = parseCustomBannerListTextElementInfo(
            json[KEY_BANNER_LIST_TEXT_LINE_1], defaults.bannerListTextLine1Info),
        .bannerListTextLine2Info = parseCustomBannerListTextElementInfo(
            json[KEY_BANNER_LIST_TEXT_LINE_2], defaults.bannerListTextLine2Info)
    };
}

CustomTheme::CustomTheme(const TCHAR* folderName, const Rgb<8, 8, 8>& primaryColor, bool darkMode,
    bool pureBlack, const ThemeColorOverrides& colorOverrides)
    : Theme(folderName, primaryColor, darkMode, pureBlack, colorOverrides)
    , _customThemeInfo(makeDefaultCustomThemeInfo(_materialColorScheme))
    , _romBrowserViewFactory(&_customThemeInfo, &_materialColorScheme, &_fontRepository)
    , _themeFileIconFactory(&_materialColorScheme, &_fontRepository) { }

void CustomTheme::LoadRomBrowserResources(const VramContext& mainVramContext, const VramContext& subVramContext)
{
    const auto file = std::make_unique<File>();
    OpenThemeFile(*file, "theme.json");

    u32 fileSize = file->GetSize();
    if (fileSize == 0)
        return;

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u8* fileDataPtr = fileData.get();

    u32 bytesRead = 0;
    if (file->Read(fileDataPtr, fileSize, bytesRead) != FR_OK)
        return;

    DynamicJsonDocument json(JSON_RESERVED_SIZE);
    if (deserializeJson(json, fileDataPtr, fileSize) != DeserializationError::Ok)
        return;

    _topBackgroundType = parseTopBackgroundType(json["topBackgroundType"].as<const char*>());
    _customThemeInfo = parseCustomThemeInfo(json, makeDefaultCustomThemeInfo(_materialColorScheme));

    mem_setVramDMapping(MEM_VRAM_D_LCDC);
    mem_setVramEMapping(MEM_VRAM_E_LCDC);
    IconButton3DView::UploadGraphics(mainVramContext);
    mem_setVramDMapping(MEM_VRAM_D_TEX_SLOT_0);
    mem_setVramEMapping(MEM_VRAM_E_TEX_PLTT_SLOT_0123);
    _romBrowserViewFactory.LoadResources(*this, mainVramContext);
}
