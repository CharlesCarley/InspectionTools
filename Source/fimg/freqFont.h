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
#ifndef _freqFont_h_
#define _freqFont_h_

#include "Math/skColor.h"
#include "Math/skScalar.h"
#include "Utils/skString.h"

struct SDL_Texture;
struct SDL_Renderer;
struct SDL_Rect;

class Font
{
public:
    struct Char
    {
        SKint32 x, y;
        SKint32 w, h;
        SKint32 o;
    };

private:
    SDL_Texture* m_texture;
    Char*        m_chars;
    SKint32      m_size;
    SKint32      m_dpi;
    SKint32      m_width;
    SKint32      m_height;
    SKint32      m_pitch;
    SKint32      m_xMax;
    SKint32      m_yMax;
    skScalar     m_pointScale;

    void setPixel(SKint32 x, SKint32 y, void* pixels, SKuint32 color) const;

    const Char& getBoundsFor(char ch) const;

public:
    Font();
    ~Font();

    void loadInternal(SDL_Renderer* renderer,
                      SKuint32      size,
                      SKuint32      dpi);

    void draw(SDL_Renderer*  renderer,
              const char*    text,
              skScalar       x,
              skScalar       y,
              const skColor& col,
              SKsize         len = SK_NPOS) const;

    void draw(SDL_Renderer*  renderer,
              skScalar       val,
              skScalar       x,
              skScalar       y,
              const skColor& col) const;

    void draw(SDL_Renderer*  renderer,
              SKint32        val,
              skScalar       x,
              skScalar       y,
              const skColor& col) const;

    inline void setPointScale(skScalar scale)
    {
        m_pointScale = scale / skScalar(m_size);
    }
};

#endif  //_freqFont_h_
