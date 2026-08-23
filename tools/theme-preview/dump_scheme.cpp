// Host-side dump of the exact palette a theme.json seed produces on the DS.
//
// The launcher generates a material theme's whole UI from two values -
// primaryColor and darkTheme - through Google's material-color-utilities,
// which is portable C++ and lives in arm9/source/material. This builds that
// same code for the host and prints the resulting MaterialColorScheme, so a
// theme can be designed without a build-copy-boot cycle on real hardware.
//
// Keep in sync with arm9/source/themes/material/MaterialColorSchemeFactory.cpp
// - that is the code this mirrors.
//
// Build/run: see run.sh in this folder.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "material/palettes/core.h"
#include "material/scheme/scheme.h"
#include "material/utils/utils.h"

using namespace material_color_utilities;

struct Field { const char* name; Argb value; };

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::fprintf(stderr,
            "usage: dump_scheme <RRGGBB> <light|dark> [name]\n");
        return 2;
    }
    unsigned rgb = (unsigned)std::strtoul(argv[1], nullptr, 16);
    bool dark = std::strcmp(argv[2], "dark") == 0 || std::strcmp(argv[2], "black") == 0;
    // "black" previews the one thing a seed alone cannot reach: the dark
    // surface tones are hardcoded to 22/24, which is a mid dark grey. Pushing
    // them near 0 is the code change a true black theme would need.
    bool black = std::strcmp(argv[2], "black") == 0;
    double tBackground = black ? 0.0 : 10.0;
    double tSurface = black ? 16.0 : 24.0;
    double tCard = black ? 12.0 : 22.0;
    const char* name = argc > 3 ? argv[3] : "";

    Argb seed = ArgbFromRgb((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
    CorePalette core = CorePalette::Of(seed);
    Scheme s = dark ? MaterialDarkColorSchemeFromPalette(core)
                    : MaterialLightColorSchemeFromPalette(core);

    // exactly the assignments MaterialColorSchemeFactory::FromPrimaryColor makes
    Field fields[] = {
        {"surfaceBright",           core.neutral().get(dark ? tSurface : 98.0)},
        {"surfaceContainerHighest", core.neutral().get(dark ? tCard : 90.0)},
        {"onSurface",               s.on_surface},
        {"onSurfaceVariant",        s.on_surface_variant},
        {"inverseOnSurface",        dark ? core.neutral().get(tBackground) : s.inverse_on_surface},
        {"outline",                 s.outline},
        {"primary",                 s.primary},
        {"onPrimary",               s.on_primary},
        {"secondaryContainer",      s.secondary_container},
        {"onSecondaryContainer",    s.on_secondary_container},
        {"tertiary",                s.tertiary},
        {"onTertiary",              s.on_tertiary},
        {"tertiaryContainer",       s.tertiary_container},
        {"onTertiaryContainer",     s.on_tertiary_container},
        {"mainIconBg",              core.secondary().get(dark ? 42.0 : 78.0)},
        {"scrim",                   core.neutral().get(dark ? 70.0 : 30.0)},
    };

    std::printf("{\"name\":\"%s\",\"seed\":\"%06X\",\"dark\":%s,\"colors\":{",
        name, rgb, dark ? "true" : "false");
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); i++)
    {
        std::printf("%s\"%s\":\"%02X%02X%02X\"", i ? "," : "", fields[i].name,
            RedFromInt(fields[i].value), GreenFromInt(fields[i].value),
            BlueFromInt(fields[i].value));
    }
    std::printf("}}\n");
    return 0;
}
