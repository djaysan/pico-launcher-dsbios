#include "common.h"
#include <algorithm>
#include "material/scheme/scheme.h"
#include "MaterialColorSchemeFactory.h"

using Rgb888 = Rgb<8, 8, 8>;

static material_color_utilities::Argb Rgb888ToMaterialArgb(const Rgb888& color)
{
    return material_color_utilities::ArgbFromRgb(color.r, color.g, color.b);
}

static Rgb888 MaterialArgbToRgb888(material_color_utilities::Argb color)
{
    return Rgb888(
        material_color_utilities::RedFromInt(color),
        material_color_utilities::GreenFromInt(color),
        material_color_utilities::BlueFromInt(color));
}

void MaterialColorSchemeFactory::FromPrimaryColor(const Rgb<8, 8, 8>& primaryColor,
    bool darkTheme, MaterialColorScheme& materialColorScheme, bool pureBlack)
{
    // Dark tones. inverseOnSurface is the one that reads as "the theme colour":
    // it is GFX_PLTT_BG_MAIN[0], the backdrop the icon cells blend against, and
    // one end of both screen gradients. surfaceBright is the icon cell face.
    // pureBlack takes the background to true black and pulls the cell face down
    // with it; the seed cannot do this, since the neutral palette's chroma is
    // clamped whatever colour it is given.
    const double backgroundTone = pureBlack ? 0.0 : 10.0;
    const double surfaceTone = pureBlack ? 16.0 : 24.0;
    const double containerTone = pureBlack ? 12.0 : 22.0;
    auto materialPrimaryColor = Rgb888ToMaterialArgb(primaryColor);
    auto corePalette = material_color_utilities::CorePalette::Of(materialPrimaryColor);
    auto scheme = darkTheme
        ? material_color_utilities::MaterialDarkColorSchemeFromPalette(corePalette)
        : material_color_utilities::MaterialLightColorSchemeFromPalette(corePalette);

    materialColorScheme.primary = MaterialArgbToRgb888(scheme.primary);
    materialColorScheme.onPrimary = MaterialArgbToRgb888(scheme.on_primary);
    materialColorScheme.secondaryContainer = MaterialArgbToRgb888(scheme.secondary_container);
    materialColorScheme.onSecondaryContainer = MaterialArgbToRgb888(scheme.on_secondary_container);
    materialColorScheme.tertiary = MaterialArgbToRgb888(scheme.tertiary);
    materialColorScheme.onTertiary = MaterialArgbToRgb888(scheme.on_tertiary);
    materialColorScheme.tertiaryContainer = MaterialArgbToRgb888(scheme.tertiary_container);
    materialColorScheme.onTertiaryContainer = MaterialArgbToRgb888(scheme.on_tertiary_container);
    materialColorScheme.inverseOnSurface = MaterialArgbToRgb888(darkTheme ? corePalette.neutral().get(backgroundTone) : scheme.inverse_on_surface);
    materialColorScheme.onSurface = MaterialArgbToRgb888(scheme.on_surface);
    materialColorScheme.onSurfaceVariant = MaterialArgbToRgb888(scheme.on_surface_variant);
    materialColorScheme.surfaceBright = MaterialArgbToRgb888(corePalette.neutral().get(darkTheme ? surfaceTone : 98.0));
    materialColorScheme.mainIconBg = MaterialArgbToRgb888(corePalette.secondary().get(darkTheme ? 42.0 : 78.0));
    // materialColorScheme.surfaceContainerLow = MaterialArgbToRgb888(corePalette.neutral().get(darkTheme ? 10.0 : 96.0));
    materialColorScheme.surfaceContainerHighest = MaterialArgbToRgb888(corePalette.neutral().get(darkTheme ? containerTone : 90.0));
    materialColorScheme.scrim = MaterialArgbToRgb888(corePalette.neutral().get(darkTheme ? 70.0 : 30.0));
    materialColorScheme.outline = MaterialArgbToRgb888(scheme.outline);
}