#include "common.h"
#include <libtwl/mem/memVram.h>
#include <libtwl/gfx/gfx.h>
#include <libtwl/gfx/gfxStatus.h>
#include <libtwl/gfx/gfxPalette.h>
#include <libtwl/gfx/gfxBackground.h>
#include "nitroFont2.h"
#include "font_nft2.h"
#include "ErrorDisplay.h"

void ErrorDisplay::PrintError(const char* errorString)
{
    mem_setVramEMapping(MEM_VRAM_E_MAIN_BG_00000);
    auto textBuffer = (u8*)0x02100000;
    memset(textBuffer, 0, 256 * 192);
    nft2_unpack((nft2_header_t*)font_nft2);
    nft2_string_render_params_t renderParams =
    {
        x: 0,
        y: 0,
        width: 256,
        height: 192
    };
    nft2_renderString((const nft2_header_t*)font_nft2, errorString, textBuffer, 256, &renderParams);
    memcpy((void*)GFX_BG_MAIN, textBuffer, 256 * 192);
    while (gfx_getVCount() != 191);
    while (gfx_getVCount() == 191);
    // 4 bit grayscale palette
    for (int i = 0; i < 16; i++)
    {
        int gray = i * 2 + (i == 0 ? 0 : 1);
        GFX_PLTT_BG_MAIN[i] = gray | (gray << 5) | (gray << 10);
    }
    REG_BG3PA = 256;
    REG_BG3PB = 0;
    REG_BG3PC = 0;
    REG_BG3PD = 256;
    REG_BG3X = 0;
    REG_BG3Y = 0;
    REG_BG3CNT = (1 << 7) | (1 << 14);
    REG_BLDCNT = 0;
    REG_DISPCNT = 3 | (1 << 11) | (1 << 16);
    REG_MASTER_BRIGHT = 0;
    GFX_PLTT_BG_SUB[0] = 0;
    REG_MASTER_BRIGHT_SUB = 0x8010;
    REG_DISPCNT_SUB = 0x10000;
    while (true);
}
