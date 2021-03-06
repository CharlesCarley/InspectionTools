/*
-------------------------------------------------------------------------------
  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------------
*/
#include <ft2build.h>
#include FT_FREETYPE_H
#include "Math/skColor.h"
#include "Math/skMath.h"
#include "freqFont.h"

#define FTINT(x) ((x) >> 6)
#define FTI64(x) ((x) << 6)

#include "DejaVu_ttf.inl"
#include "SDL.h"

const SKint32    CharStart = 32;
const SKint32    CharEnd   = 127;
const SKint32    CharTotal = CharEnd - CharStart;
const SKint32    Spacer    = 3;
const Font::Char NullChar  = {0, 0, 0, 0, 0};

Font::Font() :
    m_texture(nullptr),
    m_chars(nullptr),
    m_size(0),
    m_dpi(0),
    m_width(0),
    m_height(0),
    m_pitch(0),
    m_xMax(0),
    m_xAvr(0),
    m_yMax(0),
    m_pointScale(1)
{
}

Font::~Font()
{
    delete[] m_chars;
    if (m_texture)
        SDL_DestroyTexture(m_texture);
}

void Font_calculateLimits(FT_Face        face,
                          const SKint32& space,
                          SKint32&       w,
                          SKint32&       h,
                          SKint32&       xMax,
                          SKint32&       yMax,
                          SKint32&       xAvr)
{
    xMax = 0;
    yMax = 0;
    xAvr = 0;

    SKint32 i;
    for (i = CharStart; i < CharEnd; ++i)
    {
        if (FT_Load_Char(face, i, FT_LOAD_RENDER))
            continue;

        FT_GlyphSlot slot = face->glyph;
        if (!slot)
            continue;

        const SKint32 cx = slot->metrics.horiBearingX + slot->metrics.width;
        const SKint32 cy = slot->metrics.height;

        if (yMax < cy)
            yMax = cy;
        if (xMax < cx)
            xMax = cx;

        xAvr += cx;
    }

    xMax = FTINT(xMax);
    yMax = FTINT(yMax);
    xAvr = FTINT(xAvr);
    xAvr /= CharTotal;

    skScalar sx, sy, sr;
    sr = skSqrt(skScalar(CharTotal));

    sx = skScalar((xMax + space) * CharTotal);
    sx /= sr;
    sx += skScalar(xMax);

    sy = skScalar((yMax + space) * CharTotal);
    sy /= sr;
    sy += skScalar(yMax);

    w = (SKint32)sx;
    h = (SKint32)sy;
}

void Font::setPixel(SKint32 x, SKint32 y, void* pixels, SKuint32 color) const
{
    if (x > m_width || y > m_height || x < 0 || y < 0 || !pixels)
        return;

    const SKuint64 o = y * (SKuint64)m_pitch;
    if (o > SK_NPOS32)
        return;

    const SKuint64 k = (SKuint64)x * 4;
    if (o > SK_NPOS32)
        return;
    auto* pix = (SKuint8*)pixels;
    auto* buf = (SKuint32*)(pix + o + k);
    *buf      = color;
}

const Font::Char& Font::getBoundsFor(char ch) const
{
    if (m_chars)
    {
        const auto idx = (SKuint8)ch;
        if (idx >= CharStart && idx <= CharEnd)
            return m_chars[idx - CharStart];
    }
    return NullChar;
}

void Font::loadInternal(SDL_Renderer* renderer,
                        SKuint32      size,
                        SKuint32      dpi)
{
    FT_Library lib;
    FT_Face    face;
    if (FT_Init_FreeType(&lib))
        return;

    if (FT_New_Memory_Face(lib, (const FT_Byte*)DejaVu_ttf, (FT_Long)DejaVu_ttf_size, 0, &face))
        return;

    if (FT_Set_Char_Size(face, FTI64(size), FTI64(size), dpi, dpi))
        return;

    SKint32 ix, iy;
    SKint32 x, y;
    SKint32 i;

    m_dpi  = dpi;
    m_size = size;

    Font_calculateLimits(face, Spacer, m_width, m_height, m_xMax, m_yMax, m_xAvr);

    m_texture = SDL_CreateTexture(renderer,
                                  SDL_PIXELFORMAT_RGBA8888,
                                  SDL_TEXTUREACCESS_STREAMING,
                                  m_width,
                                  m_height);

    SDL_SetTextureBlendMode(m_texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(m_texture, SDL_ScaleModeLinear);

    void* pixels;
    SDL_LockTexture(m_texture, nullptr, &pixels, &m_pitch);

    m_chars = new Char[CharTotal];
    memset(m_chars, 0, sizeof(Char) * CharTotal);

    x = y = 0;
    for (i = CharStart; i < CharEnd; ++i)
    {
        if (FT_Load_Char(face, i, FT_LOAD_NO_HINTING | FT_LOAD_RENDER))
            continue;

        if (!face->glyph)
            continue;

        SKuint8* imgBuf = face->glyph->bitmap.buffer;
        if (!imgBuf)
            continue;

        FT_GlyphSlot slot = face->glyph;
        if (!slot)
            continue;

        const SKint32 yBearing = FTINT(slot->metrics.horiBearingY);
        const SKint32 xBearing = FTINT(slot->metrics.horiBearingX);
        const SKint32 advance  = FTINT(slot->advance.x);

        for (iy = 0; iy < slot->bitmap.rows && imgBuf; ++iy)
        {
            const SKint32 ny = y + iy;

            for (ix = 0; ix < slot->bitmap.width; ix++)
            {
                const SKint32 nx = x + ix + xBearing;
                const SKuint8 ch = *imgBuf++;
                if (ch != 0)
                    setPixel(nx, ny, pixels, 0xFFFFFFFF);
                else
                    setPixel(nx, ny, pixels, 0x00000000);
            }
        }

        Char& ch = m_chars[i - CharStart];

        ch.x = x;
        ch.y = y;
        ch.w = advance;
        ch.h = m_yMax;
        ch.o = skMax<SKint32>(0, m_yMax - yBearing);

        const SKint32 nx = m_xMax + Spacer;
        x += nx;
        if (x + nx > m_width)
        {
            x = 0;
            y += m_yMax + Spacer;
        }
    }

    SDL_UnlockTexture(m_texture);
    FT_Done_Face(face);
    FT_Done_FreeType(lib);
}

void Font::setColor(const skColor& col)
{
    m_color = col;
}

void Font::setColor(const skColori& col)
{
    m_color = skColor(col);
}

void Font::getMaxExtent(skVector2& dest, const SKuint32 val) const
{
    char buf[33];

    const SKsize len = (SKsize)skSprintf(buf, 32, "%u", val);
    getMaxExtent(dest, buf, len);
}

void Font::getMaxExtent(skVector2& dest, const SKint32 val) const
{
    char buf[33];

    const SKsize len = (SKsize)skSprintf(buf, 32, "%d", val);
    getMaxExtent(dest, buf, len);
}

void Font::getMaxExtent(skVector2& dest, const skScalar val) const
{
    char buf[33];

    const double v   = double(val);
    const SKsize len = (SKsize)skSprintf(buf, 32, "%0.2f", v);
    getMaxExtent(dest, buf, len);
}

void Font::getMaxExtent(skVector2& dest, const char* text, SKsize len) const
{
    if (!text || !*text)
        return;

    if (len == SK_NPOS)
        len = strlen(text);

    dest.x = 0;
    dest.y = skScalar(m_yMax) * m_pointScale;

    for (SKsize i = 0; i < len; ++i)
    {
        const SKuint8 ch = text[i];
        if (ch == '\n' || ch == '\r')
            dest.y += skScalar(m_yMax) * m_pointScale;
        else if (ch == ' ')
            dest.x += skScalar(m_xMax) * m_pointScale;
        else if (ch == '\t')
            dest.x += 4 * skScalar(m_xMax) * m_pointScale;
        else
        {
            const Char& cch = getBoundsFor(ch);
            if (cch.w > 0)
                dest.x += skScalar(cch.w) * m_pointScale;
        }
    }
}

void Font::draw(SDL_Renderer* renderer,
                const char*   text,
                skScalar      x,
                skScalar      y,
                SKsize        len) const
{
    if (!text || !*text)
        return;

    if (len == SK_NPOS)
        len = strlen(text);

    SDL_Rect       dst{};
    const skScalar xOffs = x;

    SKuint8 r, g, b;
    m_color.asRGB888(r, g, b);
    SDL_SetTextureColorMod(m_texture, r, g, b);

    for (SKsize i = 0; i < len; ++i)
    {
        const SKuint8 ch = text[i];

        if (ch == '\n' || ch == '\r')
        {
            y += skScalar(m_yMax) * m_pointScale;
            x = xOffs;
        }
        else if (ch == ' ')
            x += skScalar(m_xMax) * m_pointScale;
        else if (ch == '\t')
            x += 4 * skScalar(m_xMax) * m_pointScale;
        else
        {
            const Char& cch = getBoundsFor(ch);
            if (cch.w > 0)
            {
                dst.x = (int)x;
                dst.y = (int)y;
                dst.y += int(skScalar(cch.o) * m_pointScale);
                dst.w = int(skScalar(cch.w) * m_pointScale);
                dst.h = int(skScalar(cch.h) * m_pointScale);

                if (dst.w > 0 && dst.h > 0)
                {
                    SDL_Rect src = {cch.x, cch.y, cch.w, cch.h};
                    SDL_RenderCopy(renderer, m_texture, &src, &dst);
                    x += skScalar(dst.w);
                }
            }
        }
    }
}

void Font::draw(SDL_Renderer*  renderer,
                const skScalar val,
                skScalar       x,
                skScalar       y) const
{
    char         buf[33];
    const double v   = double(val);
    const SKsize len = (SKsize)skSprintf(buf, 32, "%0.2f", v);
    draw(renderer, buf, x, y, len);
}

void Font::draw(SDL_Renderer* renderer,
                const SKint32 val,
                skScalar      x,
                skScalar      y) const
{
    char         buf[33];
    const SKsize len = (SKsize)skSprintf(buf, 32, "%d", val);
    draw(renderer, buf, x, y, len);
}

void Font::draw(SDL_Renderer*  renderer,
                const SKuint32 val,
                skScalar       x,
                skScalar       y) const
{
    char         buf[33];
    const SKsize len = (SKsize)skSprintf(buf, 32, "%u", val);
    draw(renderer, buf, x, y, len);
}
