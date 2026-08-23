#pragma once
#include "core/math/Rgb.h"
#include "MaterialColorScheme.h"

class MaterialColorSchemeFactory
{
public:
    /// @param pureBlack Only meaningful with \p darkTheme. Material's dark
    ///                  surfaces are tone 22-24, a mid grey; this drops them
    ///                  near the bottom of the neutral palette instead, which
    ///                  no seed colour can do on its own.
    static void FromPrimaryColor(const Rgb<8, 8, 8>& primaryColor,
        bool darkTheme, MaterialColorScheme& materialColorScheme,
        bool pureBlack = false);
};