#include "common.h"
#include "core/mini-printf.h"
#include "material/MaterialColorSchemeFactory.h"
#include "Theme.h"

// The single constructor BOTH material and custom themes run through, which is
// why applying the overrides here covers every theme type at once.
Theme::Theme(const TCHAR* folderName, const Rgb<8, 8, 8>& primaryColor, bool darkMode,
    bool pureBlack, const ThemeColorOverrides& colorOverrides)
    : _folderName(folderName)
{
    MaterialColorSchemeFactory::FromPrimaryColor(primaryColor, darkMode, _materialColorScheme,
        pureBlack);
    ApplyColorOverrides(colorOverrides);
}

// Applied AFTER the seed derives, never instead of it: an unlisted role keeps
// the value the seed gave it, so a theme.json can override one colour and stay
// coherent. The table addresses the struct fields directly rather than going
// through MaterialColorScheme::GetColor(), which only maps ten of the sixteen.
void Theme::ApplyColorOverrides(const ThemeColorOverrides& overrides)
{
    if (!overrides.mask)
        return;

    using O = ThemeColorOverrides;
    Rgb<8, 8, 8>* const fields[O::RoleCount] =
    {
        &_materialColorScheme.primary,
        &_materialColorScheme.onPrimary,
        &_materialColorScheme.secondaryContainer,
        &_materialColorScheme.onSecondaryContainer,
        &_materialColorScheme.tertiary,
        &_materialColorScheme.onTertiary,
        &_materialColorScheme.tertiaryContainer,
        &_materialColorScheme.onTertiaryContainer,
        &_materialColorScheme.surfaceBright,
        &_materialColorScheme.inverseOnSurface,
        &_materialColorScheme.onSurface,
        &_materialColorScheme.onSurfaceVariant,
        &_materialColorScheme.mainIconBg,
        &_materialColorScheme.surfaceContainerHighest,
        &_materialColorScheme.scrim,
        &_materialColorScheme.outline,
    };

    for (int i = 0; i < O::RoleCount; i++)
    {
        if (overrides.Has((O::Role)i))
            *fields[i] = overrides.colors[i];
    }
}

bool Theme::OpenThemeFile(File& file, const TCHAR* subPath) const
{
    TCHAR pathBuffer[128];
    mini_snprintf(pathBuffer, sizeof(pathBuffer), "/_pico/themes/%s/%s", _folderName.GetString(), subPath);
    return file.Open(pathBuffer, FA_OPEN_EXISTING | FA_READ) == FR_OK;
}
