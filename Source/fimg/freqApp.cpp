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
const skColor  LineColor        = skColor(0x5EF6C4FF);
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
    skVector2        m_imageSize;
    skVector2        m_mco;
    skScalar         m_mapCell;
    skScalar         m_mapCellSq;

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
        m_mco(0, 0),
        m_mapCell(1),
        m_mapCellSq(1)
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
        x = m_displayRect.x;
        y = m_displayRect.y;
    }

    void setScale(const skScalar fac)
    {
        m_scale += fac;

        if (m_scale > m_displayRect.width * m_displayRect.width)
            m_scale = m_displayRect.width * m_displayRect.width;

        if (m_scale < -1)
            m_scale = -1;
    }

    inline skScalar offsX() const
    {
        return m_origin.x + m_pan.x;
    }

    inline skScalar offsY() const
    {
        return m_origin.y + m_pan.y;
    }

    inline skScalar screenX(const skScalar& sx) const
    {
        return (sx - offsX()) * m_zoom;
    }

    inline skScalar screenY(const skScalar& sy) const
    {
        return (sy - offsY()) * m_zoom;
    }

    inline skScalar viewX(const skScalar& vx) const
    {
        return (vx / m_zoom) + offsX();
    }

    inline skScalar viewY(const skScalar& vy) const
    {
        return (vy / m_zoom) + offsY();
    }

    void screenToView(const skScalar& sx, const skScalar& sy, skScalar& vx, skScalar& vy) const
    {
        vx = screenX(sx);
        vy = screenY(sy);
    }

    void viewToScreen(const skScalar& vx, const skScalar& vy, skScalar& sx, skScalar& sy) const
    {
        sx = viewX(sx);
        sy = viewY(sy);
    }

    void viewToScreen(const skVector2& vc, skVector2& sc) const
    {
        viewToScreen(vc.x, vc.y, sc.x, sc.y);
    }

    void screenToView(const skVector2& sc, skVector2& vc) const
    {
        screenToView(sc.x, sc.y, vc.x, vc.y);
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

    void strokeRect(const skRectangle& rect) const
    {
        skScalar x0, y0, x1, y1;
        rect.getBounds(x0, y0, x1, y1);

        x0 = viewX(x0);
        x1 = viewX(x1);
        y0 = viewY(y0);
        y1 = viewY(y1);

        const SDL_Rect r = {
            (int)x0,
            (int)y0,
            (int)(x1 - x0),
            (int)(y1 - y0),
        };
        SDL_RenderDrawRect(m_renderer, &r);
    }

    void fillRect(const skRectangle& rect) const
    {
        skScalar x0, y0, x1, y1;
        rect.getBounds(x0, y0, x1, y1);

        x0 = viewX(x0);
        x1 = viewX(x1);
        y0 = viewY(y0);
        y1 = viewY(y1);

        const SDL_Rect r = {
            (int)x0,
            (int)y0,
            (int)(x1 - x0),
            (int)(y1 - y0),
        };
        SDL_RenderFillRect(m_renderer, &r);
    }

    void mapRect(skScalar x0, skScalar y0, skScalar w, skScalar h, int x, int y) const
    {
        skScalar x1 = x0 + w, y1 = y0 + h;

        x0 = viewX(x0);
        x1 = viewX(x1);
        y0 = viewY(y0);
        y1 = viewY(y1);

        skScalar u0, v0, u1, v1;
        u0 = skScalar(x) * m_mapCell;
        v0 = skScalar(y) * m_mapCell;
        u1 = u0 + m_mapCellSq;
        v1 = v0 + m_mapCellSq;

        const SDL_Rect srct{
            (int)u0,
            (int)v0,
            (int)(u1),
            (int)(v1),
        };

        const SDL_Rect dest = {
            (int)x0,
            (int)y0,
            (int)(x1 - x0),
            (int)(y1 - y0),
        };

        skPixel p(LineColor);
        SDL_SetTextureColorMod(m_texture, p.r, p.g, p.b);
        SDL_RenderCopy(m_renderer, m_texture, &srct, &dest);
    }

    void fillScreenRect(const skRectangle& rect) const
    {
        const SDL_Rect r = {
            (int)rect.x,
            (int)rect.y,
            (int)rect.width,
            (int)rect.height,
        };
        SDL_RenderFillRect(m_renderer, &r);
    }

    void strokeScreenRect(const skRectangle& rect) const
    {
        const SDL_Rect r = {
            (int)rect.x,
            (int)rect.y,
            (int)rect.width,
            (int)rect.height,
        };
        SDL_RenderDrawRect(m_renderer, &r);
    }

    void lineTo(const skScalar& x0, const skScalar& y0, const skScalar& x1, const skScalar& y1) const
    {
        SDL_RenderDrawLine(m_renderer, (int)x0, (int)y0, (int)x1, (int)y1);
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

    void fillBackImage()
    {
        skRectangle r;
        r.setSize(m_imageSize.x, m_imageSize.y);
        skVector2 xy0, xy1, xys0, xys1;
        r.getBounds(xy0, xy1);

        screenToView(xy0, xys0);
        screenToView(xy1, xys1);

        SDL_Rect srct, drct;
        srct = {
            (int)xys0.x,
            (int)xys0.y,
            (int)(xys1.x - xys0.x),
            (int)(xys1.y - xys0.y),
        };

        drct = {
            (int)m_displayRect.x,
            (int)m_displayRect.y,
            (int)m_displayRect.width,
            (int)m_displayRect.height,
        };

        skPixel p(LineColor);
        SDL_SetTextureColorMod(m_texture, p.r, p.g, p.b);
        SDL_RenderCopy(m_renderer, m_texture, &srct, &drct);
    }

    void fillBackDrop()
    {
        skScalar xMin, xMax, yMin, yMax, iter, step, vStp, vMin, vMax;
        m_displayRect.getBounds(xMin, yMin, xMax, yMax);

        iter = m_parent->m_max / m_zoom;

        setColor(Red);



        for (int i=0; i<20; ++i)
        {
            mapRect(0, i * m_mapCellSq, m_mapCellSq, m_mapCellSq, 0, i);
        
        }
        setColor(BackgroundGraph);

        m_font->setPointScale(12);
        m_font->draw(m_renderer, iter, 20, 30, White);

        step = yMin - skFmod((yMin - offsY()), iter);

        while (step < yMax)
        {
            vMin = xMin;
            vMax = xMax;
            vStp = step;

            lineTo(vMin, vStp, vMax, vStp);
            step += iter;
        }

        step = xMin - skFmod(xMin - offsX(), iter);
        while (step < xMax)
        {
            vMin = yMin;
            vMax = yMax;
            vStp = step;

            lineTo(vStp, vMin, vStp, vMax);
            step += iter;
        }


        setColor(LineColor);
        strokeScreenRect(m_displayRect);
    }

    void render()
    {
        clear(Background);

        skRectangle scaledRect(m_displayRect);
        scaledRect.expand(m_scale, m_scale);

        m_zoom = scaledRect.width / m_displayRect.width;

        fillBackDrop();
        //fillBackImage();

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

        m_imageSize.x = m_parent->m_image->getWidth();
        m_imageSize.y = m_parent->m_image->getHeight();

        skScalar ms = skMin<skScalar>(m_size.x, m_size.y);

        m_displayRect.width  = ms;
        m_displayRect.height = ms;
        m_displayRect.x      = (m_size.x - m_displayRect.width) / 2;
        m_displayRect.y      = (m_size.y - m_displayRect.height) / 2;

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

        m_mapCell   = skScalar(m_parent->m_max);
        m_mapCellSq = m_mapCell * m_mapCell;

        m_yAxisScale = skScalar(1) / skScalar(m_parent->m_max);
        m_xAxisScale = m_yAxisScale;

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
