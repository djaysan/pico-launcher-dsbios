#pragma once
#include "../Theme.h"
#include "../DefaultFontRepository.h"
#include "DsBiosMainBackground.h"
#include "romBrowser/Theme/DsBios/DsBiosThemeFileIconFactory.h"
#include "romBrowser/Theme/DsBios/DsBiosRomBrowserViewFactory.h"
#include "../custom/CustomThemeInfo.h"
#include "DsBiosSubBackground.h"

/// @brief Lohkas's DS BIOS look - clock, calendar, status bar - rebuilt on this
///        fork. Everything the fork adds (favourites, play time, recents, cheats,
///        guides) lives in the view models and gamedata.json, not in a theme, so
///        a new theme type inherits all of it and only decides how the browser is
///        drawn. Only the top screen is ours; the bottom screen is Material's.
class DsBiosTheme : public Theme
{
public:
    DsBiosTheme(const TCHAR* folderName, const Rgb<8, 8, 8>& primaryColor, bool darkMode,
        bool pureBlack = false,
        const ThemeColorOverrides& colorOverrides = ThemeColorOverrides())
        : Theme(folderName, primaryColor, darkMode, pureBlack, colorOverrides)
        , _themeFileIconFactory(&_materialColorScheme, &_fontRepository)
        , _customThemeInfo(MakeGameBoxLayout(_materialColorScheme))
        , _romBrowserViewFactory(&_customThemeInfo, &_materialColorScheme, &_fontRepository) { }

    const IFontRepository* GetFontRepository() const override
    {
        return &_fontRepository;
    }

    const IThemeFileIconFactory* GetThemeFileIconFactory() const override
    {
        return &_themeFileIconFactory;
    }

    const IRomBrowserViewFactory* GetRomBrowserViewFactory() const override
    {
        return &_romBrowserViewFactory;
    }

    std::unique_ptr<IThemeBackground> CreateRomBrowserTopBackground() const override
    {
        return std::make_unique<DsBiosSubBackground>(&_materialColorScheme);
    }

    std::unique_ptr<IThemeBackground> CreateRomBrowserBottomBackground() const override
    {
        return std::make_unique<DsBiosMainBackground>(&_materialColorScheme);
    }

    void LoadRomBrowserResources(const VramContext& mainVramContext, const VramContext& subVramContext) override;

private:
    DefaultFontRepository _fontRepository;
    DsBiosThemeFileIconFactory _themeFileIconFactory;
    CustomThemeInfo _customThemeInfo;
    DsBiosRomBrowserViewFactory _romBrowserViewFactory;

    static CustomThemeInfo MakeGameBoxLayout(const MaterialColorScheme& scheme);
};
