#include "common.h"
#include <memory>
#include <libtwl/mem/memVram.h>
#include "gui/Gx.h"
#include "gui/GraphicsContext.h"
#include "gui/IVramManager.h"
#include "gui/VramContext.h"
#include "core/math/ColorConverter.h"
#include "../ITheme.h"
#include "DsBiosMainBackground.h"

#define BLOCK_VTX_PACK(x, y, z) (((x)&0x3FF) | ((((y) >> 3) & 0x3FF) << 10) | ((z) << 20))

// One cell of the paper. The pattern repeats every 16px in both axes, so a
// single 16x16 texture with repeat on covers the screen - the same rule the top
// screen's bitmap follows, kept in step with it by construction.
#define CELL            16
#define PLTT_PAGE       0
#define PLTT_HATCH      1
#define PLTT_RULE       2

void DsBiosMainBackground::LoadResources(const ITheme& theme, const VramContext& vramContext)
{
    const auto texVram = vramContext.GetTexVramManager();
    const auto plttVram = vramContext.GetTexPlttVramManager();
    if (!texVram || !plttVram)
        return;

    u8 texture[CELL * CELL / 2];
    for (int y = 0; y < CELL; y++)
    {
        for (int x = 0; x < CELL; x += 2)
        {
            auto indexAt = [y] (int px)
            {
                if (px == CELL - 1 || y == CELL - 1)
                    return PLTT_RULE;
                return (y & 1) ? PLTT_HATCH : PLTT_PAGE;
            };
            texture[(y * CELL + x) >> 1] = indexAt(x) | (indexAt(x + 1) << 4);
        }
    }

    const Rgb8& page = _materialColorScheme->inverseOnSurface;
    const Rgb8& ink = _materialColorScheme->outline;
    auto blend = [] (const Rgb8& a, const Rgb8& b, int num, int den)
    {
        return Rgb8(a.r + (b.r - a.r) * num / den,
                    a.g + (b.g - a.g) * num / den,
                    a.b + (b.b - a.b) * num / den);
    };
    u16 palette[16] = { };
    palette[PLTT_PAGE] = ColorConverter::ToGBGR565(page);
    palette[PLTT_HATCH] = ColorConverter::ToGBGR565(blend(page, ink, 1, 8));
    palette[PLTT_RULE] = ColorConverter::ToGBGR565(blend(page, ink, 1, 4));

    // texture and palette vram are mapped to the 3d engine, so they have to be
    // swung back to LCDC to be written and swung back again afterwards
    mem_setVramDMapping(MEM_VRAM_D_LCDC);
    mem_setVramEMapping(MEM_VRAM_E_LCDC);
    _texVramOffset = texVram->Alloc(sizeof(texture));
    memcpy((void*)texVram->GetVramAddress(_texVramOffset), texture, sizeof(texture));
    _plttVramOffset = plttVram->Alloc(sizeof(palette));
    memcpy((void*)plttVram->GetVramAddress(_plttVramOffset), palette, sizeof(palette));
    mem_setVramDMapping(MEM_VRAM_D_TEX_SLOT_0);
    mem_setVramEMapping(MEM_VRAM_E_TEX_PLTT_SLOT_0123);
}

void DsBiosMainBackground::Draw(GraphicsContext& graphicsContext)
{
    Gx::MtxIdentity();
    Gx::PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_FRONT,
        false, false, false, GX_DEPTH_FUNC_LESS, false, 31, 0);
    Gx::TexImageParam(_texVramOffset >> 3, true, true, false, false, GX_TEXSIZE_16,
        GX_TEXSIZE_16, GX_TEXFMT_PLTT16, false, GX_TEXGEN_NONE);
    Gx::TexPlttBase(_plttVramOffset >> 4);
    // modulate against white, so the texture palette's own colours come through
    // rather than being tinted by whatever vertex colour was last set
    Gx::Color(31, 31, 31);
    Gx::Begin(GX_PRIMITIVE_QUAD);
    Gx::TexCoord(0, 0);
    REG_GX_VTX_10 = BLOCK_VTX_PACK(0, 0, 500);
    Gx::TexCoord(0, 192);
    REG_GX_VTX_10 = BLOCK_VTX_PACK(0, 192, 500);
    Gx::TexCoord(256, 192);
    REG_GX_VTX_10 = BLOCK_VTX_PACK(256, 192, 500);
    Gx::TexCoord(256, 0);
    REG_GX_VTX_10 = BLOCK_VTX_PACK(256, 0, 500);
    Gx::End();
}
