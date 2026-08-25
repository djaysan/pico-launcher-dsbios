#pragma once
#include "../background/IThemeBackground.h"

class MaterialColorScheme;

/// @brief Bottom screen of the DS BIOS theme: the same squared paper as the top
///        screen, drawn as one repeating 16x16 texture on the background quad.
///        It has to be a texture rather than a tiled bg - the main engine's BG0
///        IS the 3d layer and the dialog presenter owns BG1 and BG2, so there is
///        no spare layer to put paper on.
class DsBiosMainBackground : public IThemeBackground
{
public:
    explicit DsBiosMainBackground(const MaterialColorScheme* materialColorScheme)
        : _materialColorScheme(materialColorScheme) { }

    void Draw(GraphicsContext& graphicsContext) override;
    void LoadResources(const ITheme& theme, const VramContext& vramContext) override;

private:
    const MaterialColorScheme* _materialColorScheme;
    u32 _texVramOffset = 0;
    u32 _plttVramOffset = 0;
};
