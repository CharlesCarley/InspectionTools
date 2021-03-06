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
#ifndef _fimgPixelMap_h_
#define _fimgPixelMap_h_

#include "Image/skImage.h"
#include "Math/skRectangle.h"

struct SDL_Texture;
struct SDL_Renderer;

class PixelMap
{
private:
    SDL_Texture*      m_texture;
    const skRectangle m_gridPos;
    skImage*          m_image;

public:
    PixelMap(skImage* image, const skRectangle& gridPos);
    ~PixelMap();

    void loadFromImage(SDL_Renderer* renderer, const skPixel& color);

    SK_INLINE SDL_Texture* getTexture() const
    {
        return m_texture;
    }

    SK_INLINE const skRectangle& getPosition() const
    {
        return m_gridPos;
    }

    SKuint8 getByte(SKuint32 x, SKuint32 y) const;
};

#endif  //_fimgPixelMap_h_
