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
#include <cstdio>
#include "Math/skColor.h"
#include "Math/skRectangle.h"
#include "Math/skVector2.h"
#include "Utils/skLogger.h"
#include "freqFont.h"
#define SDL_MAIN_HANDLED
#include "SDL.h"

const skColor  Clear            = skColor(0x555555FF);
const skColor  Background       = skColor(0x282828FF);
const skColor  BackgroundGraph  = skColor(0x333333FF);
const skColor  BackgroundGraph2 = skColor(0x444444FF);
const skColor  BackgroundGraph3 = skColor(0x444444FF);
const skColor  LineColor        = skColor(0x5EC4F6FF);
const skColor  Background2      = skColor(0x181818FF);
const skColor  Text             = skColor(0x808080FF);
const skScalar StepScale        = skScalar(0.125);

class PrivateApp
{
private:
    SDL_Window*      m_window;
    SDL_Renderer*    m_renderer;
    Font*            m_font;
    FreqApplication* m_parent;
    bool             m_quit;
    bool             m_redraw;
    bool             m_showGrid;
    bool             m_leftIsDown;
    bool             m_ctrlDown;
    skRectangle      m_displayRect;
    skScalar         m_zoom;
    skScalar         m_scale;
    skVector2        m_pan;
    skVector2        m_origin;
    skScalar         m_xAxisScale;
    skScalar         m_yAxisScale;
    skVector2        m_displayOffs;

public:
    PrivateApp(FreqApplication* parent) :
        m_window(nullptr),
        m_renderer(nullptr),
        m_font(nullptr),
        m_parent(parent),
        m_quit(false),
        m_redraw(true),
        m_showGrid(false),
        m_leftIsDown(false),
        m_ctrlDown(false),
        m_displayRect(0, 0, 0, 0),
        m_zoom(32),
        m_scale(0),
        m_xAxisScale(1),
        m_yAxisScale(1),
        m_displayOffs(0, 0)
    {
    }

    ~PrivateApp()
    {
        delete m_font;

        if (m_renderer)
            SDL_DestroyRenderer(m_renderer);

        if (m_window)
            SDL_DestroyWindow(m_window);

        SDL_Quit();
    }

    void getOrigin(skScalar& x, skScalar& y)
    {
        x = m_displayRect.x + m_displayOffs.x;
        y = m_displayRect.y + m_displayRect.height - m_displayOffs.y;
    }

    void setScale(const skScalar fac)
    {
        m_scale += fac;

        if (m_scale > m_displayRect.width * 5)
            m_scale = m_displayRect.width * 5;

        if (m_scale < -1)
            m_scale = -1;
    }

    void setColor(const skColor& color) const
    {
        SKuint8 r, g, b, a;
        color.asInt8(r, g, b, a);
        SDL_SetRenderDrawColor(m_renderer, r, g, b, a);
    }

    void clear(const skColor& color) const
    {
        SKuint8 r, g, b, a;
        color.asInt8(r, g, b, a);

        SDL_SetRenderDrawColor(m_renderer, r, g, b, a);
        SDL_RenderClear(m_renderer);
    }

    void strokeRect(const skRectangle& rect, const skColor& color) const
    {
        setColor(color);
        const SDL_Rect r = {
            (int)rect.x,
            (int)rect.y,
            (int)rect.width,
            (int)rect.height,
        };
        SDL_RenderDrawRect(m_renderer, &r);
    }

    void fillRect(const skRectangle& rect) const
    {
        const SDL_Rect r = {
            (int)rect.x,
            (int)rect.y,
            (int)rect.width,
            (int)rect.height,
        };
        SDL_RenderFillRect(m_renderer, &r);
    }

    void fillRect2(const skRectangle& rect, const skColor& color) const
    {
        skRectangle tr(rect);
        tr.move(m_origin.x, m_origin.y);
        tr.move(m_pan.x, m_pan.y);
        tr.width /= m_zoom;
        tr.height /= m_zoom;

        skScalar x1, y1, x2, y2;
        tr.getBounds(x1, y1, x2, y2);

        y1 -= tr.height;
        y2 -= tr.height;

        setColor(color);
        const SDL_Rect r = {
            (int)x1,
            (int)y1,
            (int)(x2 - x1),
            (int)(y2 - y1),
        };
        SDL_RenderFillRect(m_renderer, &r);
    }

    void displayLine(const skVector2& f, const skVector2& t) const
    {
        setColor(LineColor);

        skVector2       fr(f.x, -f.y), to(t.x, -t.y);
        const skVector2 offs(m_origin + m_pan);

        if (m_zoom > 0)
        {
            fr /= m_zoom;
            to /= m_zoom;
        }

        fr += offs;
        to += offs;

        if (m_displayRect.contains(fr) || m_displayRect.contains(to))
        {
            SDL_RenderDrawLineF(m_renderer,
                                fr.x,
                                fr.y,
                                to.x,
                                to.y);
        }
    }

    void fillGrid(const skScalar& xMin,
                  const skScalar& xMax,
                  const skScalar& yMin,
                  const skScalar& yMax,
                  const skColor&  color) const
    {
        setColor(color);

        const skScalar sxt = ((xMax - xMin) / m_zoom) * StepScale;
        const skScalar syt = ((yMax - yMin) / m_zoom) * StepScale;

        skScalar       stp;
        const skScalar xOffset = m_origin.x + m_pan.x;
        const skScalar yOffset = m_origin.y + m_pan.y;

        stp = xMin + skFmod(xMin + xOffset, sxt);
        while (stp < xMax)
        {
            if (m_displayRect.containsX(stp))
                SDL_RenderDrawLineF(m_renderer, stp, yMin, stp, yMax);
            stp += sxt;
        }

        stp = yMin + skFmod(yMin + yOffset, syt);
        while (stp < yMax)
        {
            if (m_displayRect.containsY(stp))
                SDL_RenderDrawLineF(m_renderer, xMin, stp, xMax, stp);
            stp += syt;
        }

        if (m_displayRect.containsX(xOffset))
        {
            setColor(BackgroundGraph2);
            SDL_RenderDrawLineF(m_renderer,
                                xOffset,
                                yMin,
                                xOffset,
                                yMax);
        }
        if (m_displayRect.containsY(yOffset))
        {
            setColor(BackgroundGraph2);
            SDL_RenderDrawLineF(m_renderer,
                                xMin,
                                yOffset,
                                xMax,
                                yOffset);
        }
    }

    void fillLabels(const skScalar& xMin,
                    const skScalar& xMax,
                    const skScalar& yMin,
                    const skScalar& yMax,
                    const skColor&  color) const
    {
        setColor(color);

        const skScalar sxt = (xMax - xMin) * StepScale;
        const skScalar syt = (yMax - yMin) * StepScale;

        const skScalar xOffset = m_origin.x + m_pan.x;
        const skScalar yOffset = m_origin.y + m_pan.y;

        const skScalar xFac = 1.f / (m_xAxisScale / m_zoom);
        const skScalar yFac = 1.f / (m_yAxisScale / m_zoom);

        fillRect(skRectangle(0, 0, m_displayOffs.x, yMax));
        fillRect(skRectangle(m_displayOffs.x,
                             m_displayRect.getBottom() - m_displayOffs.y,
                             xMax,
                             m_displayOffs.y));

        m_font->setPointScale(12);

        skScalar stp;
        stp = xMin + skFmod(xMin + xOffset, sxt);
        while (stp < xMax)
        {
            if (stp > m_displayOffs.x)
            {
                m_font->draw(m_renderer,
                             (int)((stp - xOffset) * xFac),
                             stp - 3,
                             yMax - 16,
                             Text);
            }

            stp += sxt;
        }

        stp = yMin + skFmod(yMin + yOffset, syt);
        while (stp < m_displayRect.getBottom() - m_displayOffs.y)
        {
            m_font->draw(m_renderer,
                         -(int)((stp - yOffset) * yFac),
                         2,
                         stp - 6,
                         Text);

            stp += syt;
        }
    }

    void processEvents()
    {
        SDL_Event evt;
        while (SDL_PollEvent(&evt))
        {
            switch (evt.type)
            {
            case SDL_KEYDOWN:
                if (evt.key.keysym.sym == SDLK_LCTRL)
                    m_ctrlDown = true;
                if (evt.key.keysym.sym == SDLK_ESCAPE)
                    m_quit = true;
                if (evt.key.keysym.sym == SDLK_c)
                {
                    m_pan = skVector2::Zero;
                    getOrigin(m_origin.x, m_origin.y);
                    m_scale  = 1;
                    m_redraw = true;
                }
                break;
            case SDL_KEYUP:
                if (evt.key.keysym.sym == SDLK_LCTRL)
                    m_ctrlDown = false;

                if (evt.key.keysym.sym == SDLK_g)
                {
                    m_showGrid = !m_showGrid;
                    m_redraw   = true;
                }
                break;
            case SDL_MOUSEWHEEL:
                if (evt.wheel.y > 0)
                    setScale(-120);
                else
                    setScale(120);
                m_redraw = true;
                break;
            case SDL_MOUSEMOTION:
                if (m_leftIsDown)
                {
                    if (m_ctrlDown)
                        setScale(skScalar(evt.motion.yrel) * 12);
                    else
                    {
                        m_pan.x += evt.motion.xrel;
                        m_pan.y += evt.motion.yrel;
                    }

                    m_redraw = true;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (evt.button.button == SDL_BUTTON_LEFT)
                    m_leftIsDown = true;
                break;
            case SDL_MOUSEBUTTONUP:
                if (evt.button.button == SDL_BUTTON_LEFT)
                    m_leftIsDown = false;
                break;
            case SDL_QUIT:
                m_quit = true;
                break;
            default:
                break;
            }
        }
    }

    void render()
    {
        clear(Clear);

        skRectangle scaledRect(m_displayRect);
        scaledRect.expand(m_scale, m_scale);

        setColor(Background);
        fillRect(m_displayRect);

        m_zoom = scaledRect.width / m_displayRect.width;

        skScalar xMin, xMax, yMin, yMax;
        m_displayRect.getBounds(xMin, yMin, xMax, yMax);

        if (m_showGrid)
        {
            fillGrid(xMin,
                     xMax,
                     yMin,
                     yMax,
                     BackgroundGraph);
        }

        skVector2 f, t;
        skScalar  x = 0;
        for (SKsize i = 0; i < 256; i++)
        {
            const skScalar y = skScalar(m_parent->m_freqBuffer[i]) * m_yAxisScale;
            if (i == 0)
            {
                f.x = x;
                f.y = y;
            }
            else
            {
                t.x = x;
                t.y = y;

                displayLine(f, t);
                f = t;
            }
            x += m_xAxisScale;
        }

        fillLabels(xMin,
                   xMax,
                   yMin,
                   yMax,
                   Background2);

        SDL_RenderPresent(m_renderer);
    }

    void run(const SKint32 w, const SKint32 h)
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            skLogf(LD_ERROR, "SDL initialization failed:\n\t%s\n", SDL_GetError());
            return;
        }

        m_window = SDL_CreateWindow("Frequency Viewer",
                                    SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED,
                                    w,
                                    h,
                                    SDL_WINDOW_SHOWN);
        if (!m_window)
        {
            skLogf(LD_ERROR, "Failed to create window:\n\t%s\n", SDL_GetError());
            return;
        }
        m_renderer = SDL_CreateRenderer(m_window,
                                        -1,
                                        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!m_renderer)
        {
            skLogf(LD_ERROR, "Failed to create renderer:\n\t%s\n", SDL_GetError());
            return;
        }

        m_displayOffs.x = 50;
        m_displayOffs.y = 20;

        m_displayRect.width  = skScalar(w);
        m_displayRect.height = skScalar(h);

        m_font = new Font();
        m_font->loadInternal(m_renderer, 32, 72);

        m_yAxisScale = m_displayRect.height / skScalar(m_parent->m_max);
        m_xAxisScale = m_displayRect.width / skScalar(256.0);

        m_showGrid = true;
        getOrigin(m_origin.x, m_origin.y);

        while (!m_quit)
        {
            processEvents();
            if (!m_redraw)
                SDL_Delay(1);
            else
            {
                render();
                m_redraw = false;
            }
        }
    }
};

void FreqApplication::main(SKint32 w, SKint32 h)
{
    PrivateApp app(this);
    app.run(w, h);
}
