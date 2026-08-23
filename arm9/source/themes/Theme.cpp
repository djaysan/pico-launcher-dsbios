#include "common.h"
#include "core/mini-printf.h"
#include "material/MaterialColorSchemeFactory.h"
#include "Theme.h"

Theme::Theme(const TCHAR* folderName, const Rgb<8, 8, 8>& primaryColor, bool darkMode,
    bool pureBlack)
    : _folderName(folderName)
{
    MaterialColorSchemeFactory::FromPrimaryColor(primaryColor, darkMode, _materialColorScheme,
        pureBlack);
}

bool Theme::OpenThemeFile(File& file, const TCHAR* subPath) const
{
    TCHAR pathBuffer[128];
    mini_snprintf(pathBuffer, sizeof(pathBuffer), "/_pico/themes/%s/%s", _folderName.GetString(), subPath);
    return file.Open(pathBuffer, FA_OPEN_EXISTING | FA_READ) == FR_OK;
}
