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
#include "fimgApp.h"
#include <cstdio>
#include "Image/skPalette.h"
#include "Math/skColor.h"
#include "Math/skRectangle.h"
#include "Math/skTransform2D.h"
#include "Math/skVector2.h"
#include "Utils/skLogger.h"
#include "Utils/skPlatformHeaders.h"
#include "freqFont.h"
#define SDL_MAIN_HANDLED
#include "SDL.h"

const skColor  Clear            = skColor(0x555555FF);
const skColor  Background       = skColor(0x282828FF);
const skColor  BackgroundGraph  = skColor(0x333333FF);
const skColor  BackgroundGraph2 = skColor(0x252525FF);
const skColor  BackgroundGraph3 = skColor(0x545454FF);
const skColor  LineColor        = skColor(0x5EC4F6FF);
const skColor  Background2      = skColor(0x181818FF);
const skColor  White            = skColor(0xFFFFFFFF);
const skColor  Black            = skColor(0x000000FF);
const skColor  Red              = skColor(0xFF0000FF);
const skColor  Text             = skColor(0x808080FF);
const skScalar StepScale        = skScalar(0.125);

class PrivateApp
{
private:
    typedef skArray<SDL_Texture*> Images;

    Images           m_textures;
    SDL_Window*      m_window;
    SDL_Renderer*    m_renderer;
    Font*            m_font;
    FimgApplication* m_parent;
    bool             m_quit;
    bool             m_redraw;
    bool             m_showGrid;
    bool             m_leftIsDown;
    bool             m_ctrlDown;
    skVector2        m_size;
    skVector2        m_extent;
    skVector2        m_displayOffs;
    skRectangle      m_displayRect;
    skRectangle      m_displayRectDisp;
    skScalar         m_zoom;
    skScalar         m_zoomMin;
    skScalar         m_scale;
    skVector2        m_origin;
    skVector2        m_view;
    skScalar         m_mapCell;
    skScalar         m_mapCellSq;

public:
    PrivateApp(FimgApplication* parent) :
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
        m_zoomMin(1),
        m_scale(0),
        m_mapCell(1),
        m_mapCellSq(1)
    {
    }

    ~PrivateApp()
    {
        delete m_font;

        Images::Iterator it = m_textures.iterator();
        while (it.hasMoreElements())
            SDL_DestroyTexture(it.getNext());

        if (m_renderer)
            SDL_DestroyRenderer(m_renderer);

        if (m_window)
            SDL_DestroyWindow(m_window);

        SDL_Quit();
    }

    void setInitial()
    {
        m_scale              = 1;
        m_displayRect.width  = skMin<skScalar>(m_size.x, m_size.y) - 40;
        m_displayRect.height = m_displayRect.width;

        m_displayRect.x = m_size.x - (m_displayRect.width + 20);
        m_displayRect.y = 20;

        m_origin.x = 0;
        m_origin.y = 0;
        m_extent.x = m_displayRect.width;
        m_extent.y = m_displayRect.width;

        int mod = (m_mapCell);
        if ((int)(m_extent.x) % mod != 0)
            m_extent.x += mod - (int)(m_extent.x) % mod;
        if ((int)(m_extent.y) % mod != 0)
            m_extent.y += mod - (int)(m_extent.y) % mod;

        m_view.x = m_extent.x / 2;
        m_view.y = m_extent.y / 2;

        m_zoomMin = m_zoom = (m_extent.x) / m_displayRect.width;
    }

    void clampScale(const skScalar fac, bool negate)
    {
        m_scale += fac * (negate ? -1 : 1);
        if (m_scale < -1.1)
            m_scale = 1.0;

        if (m_scale > skSqu(m_mapCellSq) / 8)
            m_scale = skSqu(m_mapCellSq) / 8;
    }

    void setScale(const skScalar factor, bool negate)
    {
        if (skEq(factor, 0))
            return;

        clampScale(factor, negate);

        if (m_scale > 1)
        {
            m_extent = m_displayRect.getSize() + m_scale;
            m_zoom   = m_extent.x / m_displayRect.width;

            if (m_zoom <= m_zoomMin)
                m_zoom = m_zoomMin;
        }
    }

    skVector2 getOrigin() const
    {
        return m_origin;
    }

    skVector2 getExtent() const
    {
        return m_extent;
    }

    inline skVector2 getOffset() const
    {
        return skVector2(offsX(), offsY());
    }

    inline void setOffset(const skVector2& offs)
    {
        m_origin = offs;
    }

    inline skScalar offsX() const
    {
        return (-m_view.x + m_origin.x + m_extent.x / 2) / m_zoom;
    }

    inline skScalar offsY() const
    {
        return (-m_view.y + m_origin.y + m_extent.y / 2) / m_zoom;
        ;
    }

    inline skScalar screenX(const skScalar& vx) const
    {
        return ((vx - offsX()) * m_zoom);
    }

    inline skScalar screenY(const skScalar& vy) const
    {
        return ((vy - offsY()) * m_zoom);
    }

    inline skVector2 view(const skVector2& sc) const
    {
        return skVector2(viewX(sc.x), viewY(sc.y));
    }
    inline skVector2 screen(const skVector2& vp) const
    {
        return skVector2(screenX(vp.x), screenY(vp.y));
    }

    inline skScalar viewX(const skScalar& sx) const
    {
        return (sx / m_zoom) + (offsX());
    }

    inline skScalar viewY(const skScalar& sy) const
    {
        return (sy / m_zoom) + offsY();
    }

    void screenToView(const skScalar& sx, const skScalar& sy, skScalar& vx, skScalar& vy) const
    {
        vx = viewX(sx);
        vy = viewY(sy);
    }

    void viewToScreen(const skScalar& vx, const skScalar& vy, skScalar& sx, skScalar& sy) const
    {
        sx = screenX(sx);
        sy = screenY(sy);
    }

    void viewToScreen(const skVector2& vc, skVector2& sc) const
    {
        viewToScreen(vc.x, vc.y, sc.x, sc.y);
    }

    void screenToView(const skVector2& sc, skVector2& vc) const
    {
        screenToView(sc.x, sc.y, vc.x, vc.y);
    }

    void setColor(const skPixel& color) const
    {
        SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
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

        x0 = viewX(x0), x1 = viewX(x1);
        y0 = viewY(y0), y1 = viewY(y1);

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

        x0 = viewX(x0), x1 = viewX(x1);
        y0 = viewY(y0), y1 = viewY(y1);

        const SDL_Rect r = {
            (int)x0,
            (int)y0,
            (int)(x1 - x0),
            (int)(y1 - y0),
        };
        SDL_RenderFillRect(m_renderer, &r);
    }

    void fillPoint(const skVector2& pt) const
    {
        skScalar x0 = pt.x, y0 = pt.y, x1 = pt.x, y1 = pt.y;

        x0 = viewX(x0 - 10), x1 = viewX(x1 + 10);
        y0 = viewY(y0 - 10), y1 = viewY(y1 + 10);

        const SDL_Rect r = {
            (int)x0,
            (int)y0,
            (int)((x1 - x0) * m_zoom),
            (int)((y1 - y0) * m_zoom),
        };
        SDL_RenderFillRect(m_renderer, &r);
    }

    void mapRect(skScalar x0, skScalar y0, skScalar w, skScalar h, int x, int y, int max) const
    {
        int idx = x * max + y;
        if (idx > (int)m_textures.size())
            return;

        SDL_Texture* tex = m_textures.at(idx);
        if (!tex)
            return;

        skScalar x1 = x0 + w, y1 = y0 + h;
        x0 = viewX(x0), x1 = viewX(x1);
        y0 = viewY(y0), y1 = viewY(y1);

        skScalar u0, v0, u1, v1;
        u0 = 0, u1 = m_mapCell;
        v0 = 0, v1 = m_mapCell;

        const SDL_Rect srct = {(int)u0, (int)v0, (int)(u1), (int)(v1)};
        const SDL_Rect dest = {(int)x0, (int)y0, (int)(x1 - x0), (int)(y1 - y0)};

        skPixel p(LineColor);
        SDL_SetTextureColorMod(tex, p.r, p.g, p.b);
        SDL_RenderCopy(m_renderer, tex, &srct, &dest);
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

    void lineToView(const skScalar& x0, const skScalar& y0, const skScalar& x1, const skScalar& y1) const
    {
        SDL_RenderDrawLine(m_renderer,
                           (int)viewX(x0),
                           (int)viewY(y0),
                           (int)viewX(x1),
                           (int)viewY(y1));
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
                if (evt.key.keysym.sym == SDLK_g)
                {
                    m_showGrid = !m_showGrid;
                    m_redraw   = true;
                }
                if (evt.key.keysym.sym == SDLK_c)
                {
                    setInitial();
                    m_redraw = true;
                }
                break;
            case SDL_KEYUP:
                if (evt.key.keysym.sym == SDLK_LCTRL)
                {
                    m_ctrlDown = false;
                    m_redraw   = true;
                }

                break;
            case SDL_MOUSEWHEEL:
                setScale(127 * m_zoom, evt.wheel.y > 0);
                m_redraw = true;
                break;
            case SDL_MOUSEMOTION:
                if (m_leftIsDown)
                {
                    if (m_ctrlDown)
                    {
                        skVector2 dv(skScalar(12 * evt.motion.xrel),
                                     skScalar(12 * evt.motion.yrel));
                        setScale(dv.length() * m_zoom, dv.y < 0);
                    }
                    else
                    {
                        m_origin.x += skScalar(evt.motion.xrel) * m_zoom;
                        m_origin.y += skScalar(evt.motion.yrel) * m_zoom;
                    }
                    m_redraw = true;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (evt.button.button == SDL_BUTTON_LEFT)
                {
                    SDL_CaptureMouse(SDL_TRUE);

                    m_leftIsDown = true;
                    m_redraw     = true;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (evt.button.button == SDL_BUTTON_LEFT)
                {
                    SDL_CaptureMouse(SDL_FALSE);
                    m_leftIsDown = false;
                }
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
        skScalar xMin, xMax, yMin, yMax, iter, step, vStp, vMin, vMax;

        xMin = 0;
        yMin = 0;
        xMax = m_displayRect.width;
        yMax = m_displayRect.height;

        bool doGrid = skEqT(m_zoom, 1, .5);

        iter   = (doGrid ? m_mapCell : m_mapCellSq) / m_zoom;
        doGrid = true;

        SDL_Rect subrect = {
            (int)m_displayRect.x,
            (int)m_displayRect.y,
            (int)m_displayRect.width,
            (int)m_displayRect.height,
        };
        SDL_RenderSetViewport(m_renderer, &subrect);

        int max = (int)skSqrt((skScalar)m_textures.size());
        for (int x = 0; x < max; ++x)
            for (int y = 0; y < max; ++y)
                mapRect(x * m_mapCellSq, y * m_mapCellSq, m_mapCellSq, m_mapCellSq, x, y, max);

        setColor(BackgroundGraph2);

        if (doGrid && m_showGrid)
        {
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
        }

        subrect = {
            (int)0,
            (int)0,
            (int)m_size.x,
            (int)m_size.y,
        };
        SDL_RenderSetViewport(m_renderer, &subrect);

        m_font->setPointScale(12);
        m_font->draw(m_renderer, iter, 20, 30, Text);

        setColor(skPalette::Grey05);
        strokeScreenRect(m_displayRect);
    }

    void render()
    {
        clear(Background2);
        fillBackDrop();
        SDL_RenderPresent(m_renderer);
    }

    void convertTextureMap(skImage* ima)
    {
        if (ima->getSizeInBytes() <= 0)
            return;

        SDL_Texture* tex = SDL_CreateTexture(m_renderer,
                                             SDL_PIXELFORMAT_ABGR8888,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             ima->getWidth(),
                                             ima->getHeight());
        if (tex)
        {
            void* pixels;
            int   pitch;

            SDL_LockTexture(tex, nullptr, &pixels, &pitch);
            if (pixels)
            {
                skMemcpy(pixels, ima->getBytes(), ima->getSizeInBytes());
                SDL_UnlockTexture(tex);

                SDL_SetTextureScaleMode(tex, SDL_ScaleModeNearest);
                SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

                m_textures.push_back(tex);
            }
        }
    }

    void buildMaps()
    {
        m_textures.reserve(m_parent->m_images.size());

        FimgApplication::Images::Iterator it = m_parent->m_images.iterator();
        while (it.hasMoreElements())
        {
            skImage* ima = it.getNext();
            convertTextureMap(ima);
            delete ima;
        }
        m_parent->m_images.clear();
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

        m_mapCell   = skScalar(m_parent->m_max);
        m_mapCellSq = m_mapCell * m_mapCell;

        setInitial();

        m_font = new Font();
        m_font->setPointScale(10);
        m_font->loadInternal(m_renderer, 55, 72);

        buildMaps();

        m_showGrid = true;

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

void FimgApplication::clear()
{
    Images::Iterator it = m_images.iterator();
    while (it.hasMoreElements())
        delete it.getNext();
    m_images.clear();
}

void FimgApplication::run(SKint32 w, SKint32 h)
{
    PrivateApp app(this);
    app.run(w, h);
}
