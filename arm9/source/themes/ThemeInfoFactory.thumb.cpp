#include "common.h"
#include "common.h"
#include <memory>
#include "fat/File.h"
#include "json/ArduinoJson.h"
#include "core/mini-printf.h"
#include "core/math/Rgb.h"
#include "ThemeInfoFactory.h"

#pragma GCC optimize("Os")

// the optional "colors" block adds sixteen members plus their strings
#define JSON_RESERVED_SIZE  4096

#define KEY_TYPE            "type"
#define KEY_NAME            "name"
#define KEY_DESCRIPTION     "description"
#define KEY_AUTHOR          "author"
#define KEY_PRIMARY_COLOR   "primaryColor"
#define KEY_COLOR_R         "r"
#define KEY_COLOR_G         "g"
#define KEY_COLOR_B         "b"
#define KEY_DARK_THEME      "darkTheme"
#define KEY_PURE_BLACK      "pureBlack"
#define KEY_COLORS          "colors"

static bool tryParseThemeType(const char* themeTypeString, ThemeType& themeType)
{
    if (!themeTypeString)
        return false;

    if (!strcasecmp(themeTypeString, "Material"))
        themeType = ThemeType::Material;
    else if (!strcasecmp(themeTypeString, "Custom"))
        themeType = ThemeType::Custom;
    else if (!strcasecmp(themeTypeString, "DsBios"))
        themeType = ThemeType::DsBios;
    else
        return false;

    return true;
}

// Order matches ThemeColorOverrides::Role exactly.
static const char* const sColorRoleNames[ThemeColorOverrides::RoleCount] =
{
    "primary", "onPrimary", "secondaryContainer", "onSecondaryContainer",
    "tertiary", "onTertiary", "tertiaryContainer", "onTertiaryContainer",
    "surfaceBright", "inverseOnSurface", "onSurface", "onSurfaceVariant",
    "mainIconBg", "surfaceContainerHighest", "scrim", "outline",
};

static bool parseHexDigit(char c, u32& out)
{
    if (c >= '0' && c <= '9') { out = c - '0'; return true; }
    if (c >= 'a' && c <= 'f') { out = c - 'a' + 10; return true; }
    if (c >= 'A' && c <= 'F') { out = c - 'A' + 10; return true; }
    return false;
}

// "#RRGGBB" or "RRGGBB". Anything else is ignored rather than guessed at, so a
// typo leaves that role on its derived value instead of turning it black.
static bool parseHexColor(const char* text, Rgb<8, 8, 8>& out)
{
    if (!text)
        return false;
    if (*text == '#')
        text++;
    u32 v[6];
    for (int i = 0; i < 6; i++)
    {
        if (!parseHexDigit(text[i], v[i]))
            return false;
    }
    if (text[6] != 0)
        return false;
    out = Rgb<8, 8, 8>((v[0] << 4) | v[1], (v[2] << 4) | v[3], (v[4] << 4) | v[5]);
    return true;
}

static ThemeColorOverrides parseColorOverrides(const JsonObjectConst& json)
{
    ThemeColorOverrides overrides;
    if (json.isNull())
        return overrides;

    for (int i = 0; i < ThemeColorOverrides::RoleCount; i++)
    {
        Rgb<8, 8, 8> color;
        if (parseHexColor(json[sColorRoleNames[i]].as<const char*>(), color))
            overrides.Set((ThemeColorOverrides::Role)i, color);
    }
    return overrides;
}

static Rgb<8, 8, 8> parseColor(const JsonObjectConst& json, const Rgb<8, 8, 8>& defaultColor)
{
    if (json.isNull())
    {
        return defaultColor;
    }

    return Rgb<8, 8, 8>(
        json[KEY_COLOR_R] | 0,
        json[KEY_COLOR_G] | 0,
        json[KEY_COLOR_B] | 0
    );
}

static std::unique_ptr<ThemeInfo> fromJson(const TCHAR* folderName, const JsonDocument& json)
{
    ThemeType themeType;
    if (!tryParseThemeType(json[KEY_TYPE].as<const char*>(), themeType))
    {
        themeType = ThemeType::Custom;
    }
    return std::make_unique<ThemeInfo>(
        folderName,
        themeType,
        json[KEY_NAME] | "",
        json[KEY_DESCRIPTION] | "",
        json[KEY_AUTHOR] | "",
        parseColor(json[KEY_PRIMARY_COLOR], Rgb<8, 8, 8>(0xFF, 0xFF, 0xFF)),
        json[KEY_DARK_THEME] | false,
        json[KEY_PURE_BLACK] | false,
        parseColorOverrides(json[KEY_COLORS])
    );
}

std::unique_ptr<ThemeInfo> ThemeInfoFactory::CreateFromThemeFolder(const TCHAR* folderName) const
{
    TCHAR pathBuffer[128];
    mini_snprintf(pathBuffer, sizeof(pathBuffer), "/_pico/themes/%s/theme.json", folderName);

    const auto file = std::make_unique<File>();
    if (file->Open(pathBuffer, FA_READ | FA_OPEN_EXISTING) != FR_OK)
    {
        return nullptr;
    }

    u32 fileSize = file->GetSize();
    if (fileSize == 0)
    {
        return nullptr;
    }

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u8* fileDataPtr = fileData.get();

    u32 bytesRead = 0;
    if (file->Read(fileDataPtr, fileSize, bytesRead) != FR_OK)
    {
        return nullptr;
    }

    DynamicJsonDocument json(JSON_RESERVED_SIZE);
    if (deserializeJson(json, fileDataPtr, fileSize) != DeserializationError::Ok)
    {
        return nullptr;
    }

    return fromJson(folderName, json);
}
