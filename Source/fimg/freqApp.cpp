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
#include "Utils/skPlatformHeaders.h"
#include "freqFont.h"
#define SDL_MAIN_HANDLED
#include "SDL.h"

const skColor  Clear            = skColor(0x555555FF);
const skColor  Background       = skColor(0x282828FF);
const skColor  BackgroundGraph  = skColor(0x333333FF);
const skColor  BackgroundGraph2 = skColor(0x444444FF);
const skColor  BackgroundGraph3 = skColor(0x545454FF);
const skColor  LineColor        = skColor(0xF65EC4FF);
const skColor  Background2      = skColor(0x181818FF);
const skColor  White            = skColor(0xFFFFFFFF);
const skColor  Black            = skColor(0x000000FF);
const skColor  Red              = skColor(0xFF0000FF);
const skColor  Text             = skColor(0x808080FF);
const skScalar StepScale        = skScalar(0.125);

class PrivateApp
{
private:
    SDL_Texture*     m_texture;
    SDL_Window*      m_window;
    SDL_Renderer*    m_renderer;
    Font*            m_font;
    FreqApplication* m_parent;
    bool             m_quit;
    bool             m_redraw;
    bool             m_showGrid;
    bool             m_leftIsDown;
    bool             m_ctrlDown;
    skVector2        m_size;
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
        m_texture(nullptr),
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

        if (m_texture)
            SDL_DestroyTexture(m_texture);

        if (m_renderer)
            SDL_DestroyRenderer(m_renderer);

        if (m_window)
            SDL_DestroyWindow(m_window);

        SDL_Quit();
    }

    void getOrigin(skScalar& x, skScalar& y)
    {
        x = m_displayRect.x + m_displayRect.width / 2;
        y = m_displayRect.y + m_displayRect.width / 2;
    }

    void setScale(const skScalar fac)
    {
        m_scale += fac;

        if (m_scale > m_displayRect.width * m_displayRect.width)
            m_scale = m_displayRect.width * m_displayRect.width;

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
                        m_pan.x -= evt.motion.xrel;
                        m_pan.y += evt.motion.yrel;

                        m_pan.x = skMax<skScalar>(m_pan.x, 0.f);
                        m_pan.x = skMin<skScalar>(m_pan.x, (skScalar)m_parent->m_image->getWidth());
                        m_pan.y = skMax<skScalar>(m_pan.y, 0.f);
                        m_pan.y = skMin<skScalar>(m_pan.y, (skScalar)m_parent->m_image->getHeight());
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

    void fillBackDrop()
    {
        skScalar xMin, xMax, yMin, yMax;
        m_displayRect.getBounds(xMin, yMin, xMax, yMax);

        setColor(Background2);
        fillRect(m_displayRect);

        setColor(Background2);

        skScalar stp = yMin;

        const skScalar sxt = (int)(m_parent->m_max * m_xAxisScale * m_zoom);
        const skScalar syt = (int)(m_parent->m_max * m_yAxisScale * m_zoom);
        m_font->setPointScale(10);

        skScalar fac = 0;

        while (stp < yMax)
        {
            SDL_RenderDrawLineF(m_renderer, 0, stp, m_size.x, stp);

            if (skEqT(skFmod(fac, 8 / m_zoom), 0, 1))
            {
                m_font->draw(m_renderer, (int)(fac * m_parent->m_max), 5, stp - 6, Text);

            
            }

            fac += 1.0;
            stp += syt;
        }

        stp = xMin;
        fac = 0;

        while (stp < xMax)
        {
            SDL_RenderDrawLineF(m_renderer, stp, 0, stp, m_size.y);
            if (skEqT(skFmod(fac, 8 / m_zoom), 0, 1))
                m_font->draw(m_renderer, (int)(fac * m_parent->m_max), stp - 7, 12, Text);

            fac += 1.0;
            stp += sxt;
        }

        strokeRect(m_displayRect, BackgroundGraph2);
    }

    void fillBackImage()
    {
        skRectangle r;
        r.setSize(m_parent->m_image->getWidth() / m_zoom,
                  m_parent->m_image->getHeight() / m_zoom);
        r.move(m_pan.x, m_pan.y);

        SDL_Rect srct, drct;
        srct = {
            (int)r.x,
            (int)r.y,
            (int)r.width,
            (int)r.height,
        };

        drct = {
            (int)m_displayRect.x,
            (int)m_displayRect.y,
            (int)m_displayRect.width,
            (int)m_displayRect.height,
        };

        skPixel p(LineColor);
        SDL_SetTextureColorMod(m_texture, p.r, p.g, p.b);
        SDL_Point pt = {(int)m_pan.x, (int)m_pan.y};
        SDL_RenderCopyEx(m_renderer, m_texture, &srct, &drct, 0, &pt, SDL_FLIP_VERTICAL);
    }

    void render()
    {
        clear(Background);
        skRectangle scaledRect(m_displayRect);
        scaledRect.expand(m_scale, m_scale);
        m_zoom = scaledRect.width / m_displayRect.width;

        fillBackDrop();
        fillBackImage();

        SDL_RenderPresent(m_renderer);
    }

    void run(const SKint32 w, const SKint32 h)
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            skLogf(LD_ERROR, "SDL initialization failed:\n\t%s\n", SDL_GetError());
            return;
        }

        m_window = SDL_CreateWindow("File Byte Viewer",
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

        m_size.x = skScalar(w);
        m_size.y = skScalar(h);

        m_displayRect.width  = skScalar(w) - 100;
        m_displayRect.height = skScalar(h) - 50;
        m_displayRect.x += 50;
        m_displayRect.y += 25;

        m_font = new Font();
        m_font->loadInternal(m_renderer, 48, 72);

        m_texture = SDL_CreateTexture(m_renderer,
                                      SDL_PIXELFORMAT_ABGR8888,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      m_parent->m_image->getWidth(),
                                      m_parent->m_image->getHeight());

        void* pixels;
        int   pitch;
        SDL_LockTexture(m_texture, nullptr, &pixels, &pitch);

        skMemcpy(pixels, m_parent->m_image->getBytes(), m_parent->m_image->getSizeInBytes());
        SDL_UnlockTexture(m_texture);

        SDL_SetTextureScaleMode(m_texture, SDL_ScaleModeNearest);
        SDL_SetTextureBlendMode(m_texture, SDL_BLENDMODE_BLEND);

        m_yAxisScale = m_displayRect.height / skScalar(m_parent->m_image->getHeight());
        m_xAxisScale = m_displayRect.width / skScalar(m_parent->m_image->getWidth());

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
