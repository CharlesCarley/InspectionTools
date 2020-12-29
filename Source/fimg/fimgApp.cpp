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
#include "Math/skScreenTransform.h"
#include "Utils/skLogger.h"
#include "Utils/skPlatformHeaders.h"
#include "drawUtils.h"
#include "freqFont.h"
#define SDL_MAIN_HANDLED
#include "SDL.h"

const skColor Clear            = skColor(0x555555FF);
const skColor Background       = skColor(0x282828FF);
const skColor BackgroundGraph  = skColor(0x333333FF);
const skColor BackgroundGraph2 = skColor(0x252525FF);
const skColor BackgroundGraph3 = skColor(0x545454FF);
const skColor LineColor        = skColor(0x5EC4F6FF);
const skColor Background2      = skColor(0x181818FF);
const skColor White            = skColor(0xFFFFFFFF);
const skColor Black            = skColor(0x000000FF);
const skColor Red              = skColor(0xFF0000FF);
const skColor Text             = skColor(0x808080FF);

class PrivateApp
{
private:
    typedef skArray<SDL_Texture*> Images;

    Images            m_textures;
    SDL_Window*       m_window;
    SDL_Renderer*     m_renderer;
    Font*             m_font;
    FimgApplication*  m_parent;
    bool              m_quit;
    bool              m_redraw;
    bool              m_showGrid;
    bool              m_leftIsDown;
    bool              m_ctrlDown;
    skVector2         m_size;
    skScalar          m_mapCell;
    skScalar          m_mapCellSq;
    skScreenTransform m_xForm;

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
        skRectangle vp;
        vp.width  = skMin<skScalar>(m_size.x, m_size.y) - 40;
        vp.height = vp.width;
        vp.x      = m_size.x - (vp.width + 20);
        vp.y      = 20;

        skMath::forceAlign(vp.width, (int)m_mapCell);
        skMath::forceAlign(vp.height, (int)m_mapCell);

        skScalar sval = (skScalar)m_textures.size();
        skMath::forceAlign(sval, (int)m_mapCell);
        sval = skSqrt(sval);

        const skScalar lim = m_mapCellSq * sval;

        m_xForm.setInitialOrigin(0, 0);
        m_xForm.setScaleLimit(1, lim);
        m_xForm.setViewport(vp);

        m_xForm.reset();
    }
    void setColor(const skPixel& color) const
    {
        SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    }

    void mapRect(skScalar       x0,
                 skScalar       y0,
                 const skScalar w,
                 const skScalar h,
                 const int      x,
                 const int      y,
                 const int      max) const
    {
        const int idx = x * max + y;
        if (idx > (int)m_textures.size())
            return;

        SDL_Texture* tex = m_textures.at(idx);
        if (!tex)
            return;

        skScalar x1 = x0 + w, y1 = y0 + h;
        x0 = m_xForm.getViewX(x0), x1 = m_xForm.getViewX(x1);
        y0 = m_xForm.getViewY(y0), y1 = m_xForm.getViewY(y1);
        skScalar u0, v0, u1, v1;
        u0 = 0, u1 = m_mapCell;
        v0 = 0, v1 = m_mapCell;

        const SDL_Rect srct = {(int)u0, (int)v0, (int)u1, (int)v1};
        const SDL_Rect dest = {(int)x0, (int)y0, (int)(x1 - x0), (int)(y1 - y0)};

        skPixel p(LineColor);
        SDL_SetTextureColorMod(tex, p.r, p.g, p.b);
        SDL_RenderCopy(m_renderer, tex, &srct, &dest);
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
                    m_xForm.reset();
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
                m_xForm.zoom(127, evt.wheel.y > 0);
                m_redraw = true;
                break;
            case SDL_MOUSEMOTION:
                if (m_leftIsDown)
                {
                    if (m_ctrlDown)
                    {
                        skVector2 dv(skScalar(12 * evt.motion.xrel),
                                     skScalar(12 * evt.motion.yrel));
                        m_xForm.zoom(dv.length(), dv.y < 0);
                    }
                    else
                    {
                        m_xForm.pan((skScalar)evt.motion.xrel, (skScalar)evt.motion.yrel);
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
        skScalar xMin, xMax, yMin, yMax, val, step, vStp, vMin, vMax;

        xMin = 0;
        yMin = 0;
        xMax = m_xForm.viewportWidth();
        yMax = m_xForm.viewportHeight();

        bool doGrid = skEqT(m_xForm.getZoom(), 1, .5);

        val    = (doGrid ? m_mapCell : m_mapCellSq) / m_xForm.getZoom();
        doGrid = true;

        DrawUtils::SetViewport(m_renderer, m_xForm);

        const int max = (int)skSqrt((skScalar)m_textures.size());
        for (int x = 0; x < max; ++x)
        {
            for (int y = 0; y < max; ++y)
            {
                mapRect(skScalar(x) * m_mapCellSq,
                        skScalar(y) * m_mapCellSq,
                        m_mapCellSq,
                        m_mapCellSq,
                        x,
                        y,
                        max);
            }
        }

        DrawUtils::SetColor(m_renderer, BackgroundGraph2);

        if (doGrid && m_showGrid)
        {
            step = yMin - skFmod(yMin - m_xForm.yOffs(), val);
            while (step < yMax)
            {
                vMin = xMin;
                vMax = xMax;
                vStp = step;

                DrawUtils::ScreenLineTo(m_renderer, vMin, vStp, vMax, vStp);
                step += val;
            }

            step = xMin - skFmod(xMin - m_xForm.xOffs(), val);
            while (step < xMax)
            {
                vMin = yMin;
                vMax = yMax;
                vStp = step;

                DrawUtils::ScreenLineTo(m_renderer, vStp, vMin, vStp, vMax);
                step += val;
            }
        }

        DrawUtils::SetViewport(m_renderer, m_size);

        m_font->setPointScale(12);
        const int iv = (int)val;
        m_font->draw(m_renderer, iv, 20, 30, Text);

        setColor(skPalette::Grey05);
        DrawUtils::StrokeScreenRect(m_renderer, m_xForm.getViewport());
    }

    void render()
    {
        DrawUtils::Clear(m_renderer, Background2);
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

        m_font = new Font();
        m_font->setPointScale(10);
        m_font->loadInternal(m_renderer, 55, 72);

        buildMaps();
        setInitial();

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

void FimgApplication::run(const SKint32 w, const SKint32 h)
{
    PrivateApp app(this);
    app.run(w, h);
}
