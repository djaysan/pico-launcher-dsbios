#pragma once
#include "common.h"
#include "core/math/Rgb.h"

/// @brief Explicit per-colour overrides from theme.json's optional "colors" block.
///
/// A material theme derives all sixteen roles from ONE seed plus darkTheme and
/// pureBlack. That reaches a lot - any hue, warm or cool, light or dark - but not
/// everything: the accent and the background always share a hue, the neutral tint
/// is pinned at chroma 4, the dark surface tones are constants, and Material
/// darkens saturated hues in light mode so a bright light-mode accent is
/// unreachable at any chroma.
///
/// These are applied AFTER the seed derives, so a theme can override one role and
/// leave the other fifteen coherent. A theme.json without the block behaves
/// exactly as before.
struct ThemeColorOverrides
{
    enum Role
    {
        Primary, OnPrimary, SecondaryContainer, OnSecondaryContainer,
        Tertiary, OnTertiary, TertiaryContainer, OnTertiaryContainer,
        SurfaceBright, InverseOnSurface, OnSurface, OnSurfaceVariant,
        MainIconBg, SurfaceContainerHighest, Scrim, Outline,
        RoleCount
    };

    Rgb<8, 8, 8> colors[RoleCount];
    u32 mask;

    constexpr ThemeColorOverrides() : colors(), mask(0) { }

    constexpr bool Has(Role role) const { return (mask & (1u << role)) != 0; }

    void Set(Role role, const Rgb<8, 8, 8>& color)
    {
        colors[role] = color;
        mask |= 1u << role;
    }
};
