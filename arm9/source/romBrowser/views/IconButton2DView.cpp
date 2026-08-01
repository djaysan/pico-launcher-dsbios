#include "common.h"
#include "gui/OamManager.h"
#include "gui/OamBuilder.h"
#include "gui/IVramManager.h"
#include "gui/GraphicsContext.h"
#include "gui/input/InputProvider.h"
#include "iconButtonSelector.h"
#include "core/math/ColorConverter.h"
#include "gui/palette/GradientPalette.h"
#include "themes/material/MaterialColorScheme.h"
#include "IconButton2DView.h"

void IconButton2DView::Draw(GraphicsContext& graphicsContext)
{
    if (!graphicsContext.IsVisible(GetBounds()))
        return;

    u32 iconPaletteRow;
    if (_isFocused || _penDown)
    {
        const auto& bgColor = _materialColorScheme->GetColor(_backgroundColor);
        const auto& selectorColor = _materialColorScheme->GetColor(GetFocusFillColor());
        u32 selectorPlttRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(bgColor, selectorColor), _position.y, _position.y + 32);
        gfx_oam_entry_t* selectorOam = graphicsContext.GetOamManager().AllocOams(1);
        OamBuilder::OamWithSize<32, 32>(
                _position.x + 3,
                _position.y + 3, _selectorVramOffset >> 7)
            .WithPalette16(selectorPlttRow)
            .WithPriority(graphicsContext.GetPriority())
            .Build(selectorOam[0]);

        iconPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
            GradientPalette(selectorColor, GetFocusIconColor()), _position.y + 8, _position.y + 24);
    }
    else
    {
        if (IsCircleBackgroundVisible())
        {
            auto circleBgColor = GetCircleBackgroundColor();
            u32 circlePaletteRow = graphicsContext.GetPaletteManager().AllocRow(
                GradientPalette(
                    _materialColorScheme->GetColor(_backgroundColor),
                    _materialColorScheme->GetColor(circleBgColor)),
                _position.y, _position.y + 32);
            gfx_oam_entry_t* selectorOam = graphicsContext.GetOamManager().AllocOams(1);
            OamBuilder::OamWithSize<32, 32>(
                    _position.x + 3,
                    _position.y + 3, _selectorVramOffset >> 7)
                .WithPalette16(circlePaletteRow)
                .WithPriority(graphicsContext.GetPriority())
                .Build(selectorOam[0]);
            iconPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
                GradientPalette(
                    _materialColorScheme->GetColor(circleBgColor),
                    GetIconColor()),
                _position.y + 8, _position.y + 24);
        }
        else
        {
            iconPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
                GradientPalette(
                    _materialColorScheme->GetColor(_backgroundColor),
                    GetIconColor()),
                _position.y + 8, _position.y + 24);
        }
    }
    gfx_oam_entry_t* iconOam = graphicsContext.GetOamManager().AllocOams(1);
    OamBuilder::OamWithSize<16, 16>(
            _position.x + 8,
            _position.y + 8, _iconVramOffset >> 7)
        .WithPalette16(iconPaletteRow)
        .WithPriority(graphicsContext.GetPriority())
        .Build(iconOam[0]);
}

IconButton2DView::VramToken IconButton2DView::UploadGraphics(IVramManager& vramManager)
{
    u32 vramOffset = vramManager.Alloc(iconButtonSelectorTilesLen);
    dma_ntrCopy32(3, iconButtonSelectorTiles, vramManager.GetVramAddress(vramOffset), iconButtonSelectorTilesLen);
    return IconButton2DView::VramToken(vramOffset);
}
