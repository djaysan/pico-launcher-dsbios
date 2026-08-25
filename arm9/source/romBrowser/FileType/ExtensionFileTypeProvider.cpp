#include "common.h"
#include <string.h>
#include "UnknownFileType.h"
#include "Nds/NdsFileType.h"
#include "Gba/GbaFileType.h"
#include "ExtensionFileTypeProvider.h"

static bool isNdsExtension(const char* extension)
{
    return !strcasecmp(extension, "nds")
        || !strcasecmp(extension, "srl")
        || !strcasecmp(extension, "dsi");
}

static bool isGbaExtension(const char* extension)
{
    return !strcasecmp(extension, "gba")
        || !strcasecmp(extension, "agb");
}

// Extensions that are unambiguously a game rom for some console. Anything
// else the user associates (the .txt that opens the guide reader) stays Misc.
// Deliberately excludes catch-alls like "bin" - being wrong here would count
// arbitrary files as games.
static bool isRomExtension(const char* extension)
{
    static const char* kRomExtensions[] = {
        "nes", "fds", "unf", "unif",              // nes / famicom
        "smc", "sfc", "fig", "swc",               // snes
        "gb", "gbc", "sgb",                       // game boy
        "gg", "sms", "sg",                        // sega 8 bit
        "md", "gen", "smd",                       // mega drive
        "pce", "sgx",                             // pc engine
        "ws", "wsc",                              // wonderswan
        "ngp", "ngc",                             // neo geo pocket
        "a26", "col", "int", "lnx", "vb",         // the rest
    };
    for (const char* romExtension : kRomExtensions)
    {
        if (!strcasecmp(extension, romExtension))
            return true;
    }
    return false;
}

ExtensionFileTypeProvider::ExtensionFileTypeProvider(const AppSettings& appSettings)
    : _appSettings(appSettings)
{
    _customFileTypes = std::make_unique_for_overwrite<CustomFileType[]>(appSettings.numberOfFileAssociations);
    for (u32 i = 0; i < appSettings.numberOfFileAssociations; i++)
    {
        const FileType* baseFileType = nullptr;
        if (isGbaExtension(appSettings.fileAssociations[i].extension))
        {
            baseFileType = &GbaFileType::sInstance;
        }
        _customFileTypes[i] = CustomFileType(&appSettings.fileAssociations[i], baseFileType,
            isRomExtension(appSettings.fileAssociations[i].extension)
                ? FileTypeClassification::Game
                : FileTypeClassification::Misc);
    }
}

const FileType* ExtensionFileTypeProvider::GetFileType(const TCHAR* path) const
{
    const char* extension = strrchr(path, '.');
    if (extension)
    {
        extension++; // skip over dot

        if (isNdsExtension(extension))
        {
            return &NdsFileType::sInstance;
        }

        for (u32 i = 0; i < _appSettings.numberOfFileAssociations; i++)
        {
            if (!strcasecmp(extension, _customFileTypes[i].GetShortName()))
            {
                return &_customFileTypes[i];
            }
        }
    }

    return &UnknownFileType::sInstance;
}