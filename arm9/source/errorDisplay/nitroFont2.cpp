#include "common.h"
#include "nitroFont2.h"

bool nft2_unpack(nft2_header_t* font)
{
    if (font->signature != NFT2_SIGNATURE)
        return false;

    font->glyphInfoPtr = (const nft2_glyph_t*)((u32)font + (u32)font->glyphInfoPtr);
    font->charMapPtr = (const nft2_char_map_entry_t*)((u32)font + (u32)font->charMapPtr);
    font->glyphDataPtr = (const u8*)((u32)font + (u32)font->glyphDataPtr);

    return true;
}

int nft2_findGlyphIdxForCharacter(const nft2_header_t* font, u16 character)
{
    const nft2_char_map_entry_t* charMapEntry = font->charMapPtr;
    while (charMapEntry->count > 0)
    {
        if (charMapEntry->startChar <= character && character < charMapEntry->startChar + charMapEntry->count)
            return charMapEntry->glyphs[character - charMapEntry->startChar];

        charMapEntry = (const nft2_char_map_entry_t*)((u32)charMapEntry + 4 + 2 * charMapEntry->count);
    }

    return 0;
}

static inline void renderGlyph(const nft2_header_t* font, const nft2_glyph_t* glyph,
    int xPos, int yPos, int width, int height, u8* dst, u32 stride)
{
    int yOffset = glyph->spacingTop;
    u32 xStart = xPos < 0 ? -xPos : 0;
    u32 yStart = yPos + yOffset < 0 ? -(yPos + yOffset) : 0;

    int xEnd = glyph->glyphWidth;
    if (xPos + xEnd > width)
    {
        // by returning we only render complete glyphs
        return;
        // old code for rendering partial glyphs
        // xEnd = width - xPos;
    }

    int yEnd = glyph->glyphHeight;
    if (yPos + yOffset + yEnd > height)
        yEnd = height - (yPos + yOffset); // allow partial glyphs in the vertical direction

    const u8* glyphData = &font->glyphDataPtr[glyph->dataOffset];
    glyphData += yStart * ((glyph->glyphWidth + 1) >> 1);
    for (int y = yStart; y < yEnd; y++)
    {
        for (int x = xStart; x < xEnd; x++)
        {
            u32 data = glyphData[x >> 1];
            if ((x & 1) == 0)
                data &= 0xF;
            else
                data >>= 4;

            if (data == 0)
                continue;

            u32 finalX = x + xPos;
            u32 finalY = y + yPos + yOffset;

            dst[finalY * stride + finalX] = data;
        }
        glyphData += (glyph->glyphWidth + 1) >> 1;
    }
}

ITCM_CODE void nft2_renderString(const nft2_header_t* font, const char* string, u8* dst, u32 stride,
    nft2_string_render_params_t* renderParams)
{
    int xPos = renderParams->x;
    int yPos = renderParams->y;
    u32 textWidth = 0;
    while (true)
    {
        char c = *string++;
        if (c == 0)
            break;
        if (c == '\n')
        {
            xPos = renderParams->x;
            yPos += font->ascend + font->descend + 1;
            if (yPos >= (int)renderParams->height)
                break;
            continue;
        }

        int glyphIdx = nft2_findGlyphIdxForCharacter(font, c);
        const nft2_glyph_t* glyph = &font->glyphInfoPtr[glyphIdx];
        xPos += glyph->spacingLeft;
        renderGlyph(font, glyph, xPos, yPos, renderParams->width, renderParams->height, dst, stride);
        xPos += glyph->glyphWidth;
        if (xPos > (int)textWidth)
            textWidth = xPos;
        xPos += glyph->spacingRight;
    }
    renderParams->textWidth = textWidth;
}
