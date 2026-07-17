#include "common.h"
#include <string.h>
#include <libtwl/mem/memVram.h>
#include "gui/Gx.h"
#include "../ITheme.h"
#include "ThemeTimeOfDay.h"
#include "CustomMainBackground.h"

#define BLOCK_VTX_PACK(x, y, z) (((x)&0x3FF) | ((((y) >> 3) & 0x3FF) << 10) | ((z) << 20))

void CustomMainBackground::Draw(GraphicsContext& graphicsContext)
{
    Gx::MtxIdentity();
    Gx::PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGON_MODE_MODULATE, GX_DISPLAY_MODE_FRONT,
        false, false, false, GX_DEPTH_FUNC_LESS, false, 31, 0);
    Gx::Color(0x7FFF);
    Gx::TexImageParam((128 * 1024) >> 3, false, false, false, false, GX_TEXSIZE_256,
        GX_TEXSIZE_256, GX_TEXFMT_DIRECT, false, GX_TEXGEN_NONE);
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

void CustomMainBackground::LoadResources(const ITheme& theme, const VramContext& vramContext)
{
    auto tmpBuf = std::unique_ptr<u8[]>(new(cache_align) u8[256 * 192 * 2]);
    const auto file = std::make_unique<File>();
    bool opened = ThemeTimeOfDay::IsNight() && theme.OpenThemeFile(*file, "bottombg_night.bin");
    if (opened || theme.OpenThemeFile(*file, "bottombg.bin"))
    {
        u32 bytesRead = 0;
        file->Read(tmpBuf.get(), 256 * 192 * 2, bytesRead);
        mem_setVramAMapping(MEM_VRAM_AB_LCDC);
        memcpy((void*)0x6800000, tmpBuf.get(), 256 * 192 * 2);
        mem_setVramAMapping(MEM_VRAM_AB_TEX_SLOT_1);
        file->Close();
    }
}
