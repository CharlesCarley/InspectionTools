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
#include "freqApp.h"
#include "Math/skColor.h"
#include "Math/skRectangle.h"
#include "Math/skVector2.h"

#define SDL_MAIN_HANDLED
#include <sdl.h>

// colors
const skColor Background = skColor(0x333333FF);
const skColor Border     = skColor(0x101010FF);
const skColor LineColor  = skColor(0x4885AFFF);

class PrivateApp
{
private:
    SDL_Window*      m_window;
    SDL_Renderer*    m_renderer;
    SDL_Surface*     m_surface;
    FreqApplication* m_parent;

public:
    PrivateApp(FreqApplication* parent) :
        m_window(nullptr),
        m_renderer(nullptr),
        m_surface(nullptr),
        m_parent(parent)
    {
    }

    ~PrivateApp()
    {
        if (m_surface)
            SDL_FreeSurface(m_surface);

        if (m_renderer)
            SDL_DestroyRenderer(m_renderer);

        if (m_window)
            SDL_DestroyWindow(m_window);
        SDL_Quit();
    }

    void setColor(const skColor& color)
    {
        SKuint8 r, g, b, a;
        color.asInt8(r, g, b, a);
        SDL_SetRenderDrawColor(m_renderer, r, g, b, a);
    }

    void strokeRect(const skRectangle& rect, const skColor& color)
    {
        setColor(color);

        skScalar x1, y1, x2, y2;
        rect.getBounds(x1, y1, x2, y2);

        const SDL_FPoint points[8] = {
            {x1, y1},
            {x2, y1},
            {x2, y1},
            {x2, y2},
            {x2, y2},
            {x1, y2},
            {x1, y2},
            {x1, y1},
        };

        SDL_RenderDrawLinesF(m_renderer, points, 8);
    }

    void run(SKint32 w, SKint32 h)
    {
        m_window = SDL_CreateWindow("Frequency Viewer",
                                    SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED,
                                    w,
                                    h,
                                    SDL_WINDOW_SHOWN);
        if (!m_window)
            return;

        m_renderer = SDL_CreateRenderer(m_window,
                                        -1,
                                        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!m_renderer)
            return;

        m_surface = SDL_GetWindowSurface(m_window);

        bool quit = false;

        skRectangle rect(0, 0, skScalar(w), skScalar(h));
        skRectangle displayRect(rect);

        displayRect.contract(20, 40);

        const skScalar moh   = (skScalar(m_parent->m_max) / displayRect.height);
        const skScalar unitX = displayRect.width / skScalar(256.0);
        const skScalar unitY = skScalar(1.0) / (moh > 0 ? moh : skScalar(2.0));

        bool pointsInit = false;

        while (!quit)
        {
            SDL_Event evt;
            while (SDL_PollEvent(&evt))
            {
                switch (evt.type)
                {
                case SDL_KEYDOWN:
                    if (evt.key.keysym.sym != SDLK_ESCAPE)
                        break;
                case SDL_QUIT:
                    quit = true;
                    break;
                }
            }

            setColor(Background);
            SDL_RenderClear(m_renderer);

            strokeRect(displayRect, Border);

            skScalar step     = displayRect.x + unitX;
            skScalar baseLine = displayRect.getBottom();

            skVector2 f, t;
            for (SKsize i = 0; i < 256; i++, step += unitX)
            {
                if (i == 0)
                {
                    f.x = step;
                    f.y = baseLine - skScalar(m_parent->m_freqBuffer[i]) * unitY;
                }
                else
                {
                    t.x = step;
                    t.y = baseLine - skScalar(m_parent->m_freqBuffer[i]) * unitY;

                    setColor(LineColor);
                    SDL_RenderDrawLineF(m_renderer, f.x, f.y, t.x, t.y);
                    f = t;
                }
            }

            SDL_RenderPresent(m_renderer);
        }
    }
};

void FreqApplication::main(SKint32 w, SKint32 h)
{
    PrivateApp app(this);
    app.run(w, h);
}
