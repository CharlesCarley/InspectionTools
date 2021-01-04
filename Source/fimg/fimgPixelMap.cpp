/*
-------------------------------------------------------------------------------
  Copyright (c) Charles Carley.

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
#include "fimgPixelMap.h"
#include "SDL.h"
#include "Utils/skLogger.h"

PixelMap::PixelMap(skImage* image, const skRectangle& gridPos) :
    m_texture(nullptr),
    m_gridPos(gridPos),
    m_image(image)
{
}

PixelMap::~PixelMap()
{
    delete m_image;

    if (m_texture)
        SDL_DestroyTexture(m_texture);
}

void PixelMap::loadFromImage(SDL_Renderer* renderer, const skPixel& color)
{
    if (m_image == nullptr)
    {
        skLogd(LD_ERROR, "invalid pixel map image\n");
        return;
    }

    if (m_texture)
    {
        skLogd(LD_ERROR, "texture has already been loaded.\n");
        return;
    }

    m_texture = SDL_CreateTexture(renderer,
                                  SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_STATIC,
                                  m_image->getWidth(),
                                  m_image->getHeight());
    if (m_texture != nullptr)
    {
        SDL_UpdateTexture(m_texture, nullptr, m_image->getBytes(), m_image->getPitch());
        SDL_SetTextureColorMod(m_texture, color.r, color.g, color.b);
        SDL_SetTextureScaleMode(m_texture, SDL_ScaleModeNearest);
        SDL_SetTextureBlendMode(m_texture, SDL_BLENDMODE_ADD);
    }
}

SKuint8 PixelMap::getByte(SKuint32 x, SKuint32 y) const
{
    SKuint8 rc = 0;
    if (m_image)
    {
        skPixel px;
        m_image->getPixel(x, m_image->getHeight() - 1 - y, px);
        rc = px.r;
    }
    return rc;
}
