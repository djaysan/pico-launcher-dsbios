#pragma once
#include "fat/ff.h"
#include "core/String.h"
#include "ITheme.h"
#include "ThemeColorOverrides.h"

class Theme : public ITheme
{
public:
    bool OpenThemeFile(File& file, const TCHAR* subPath) const override;
    void LoadRomBrowserResources(const VramContext& mainVramContext, const VramContext& subVramContext) override { };
    const MaterialColorScheme& GetMaterialColorScheme() const override { return _materialColorScheme; }

protected:
    Theme(const TCHAR* folderName, const Rgb<8, 8, 8>& primaryColor, bool darkMode,
        bool pureBlack = false,
        const ThemeColorOverrides& colorOverrides = ThemeColorOverrides());

    MaterialColorScheme _materialColorScheme;

    void ApplyColorOverrides(const ThemeColorOverrides& overrides);

private:
    String<TCHAR, 64> _folderName;
};
