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
#include "fimgPixelMap.h"

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
    typedef skArray<PixelMap*> Images;

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
            delete it.getNext();

        if (m_renderer)
            SDL_DestroyRenderer(m_renderer);

        if (m_window)
            SDL_DestroyWindow(m_window);

        SDL_Quit();
    }

    void setInitial()
    {
        skRectangle vp;
        vp.width  = skMin<skScalar>(m_size.x, m_size.y);
        vp.height = vp.width;

        vp.width -= 32;
        vp.height -= 32;

        vp.x = m_size.x - (vp.width + 16);
        vp.y = m_size.y - (vp.width + 16);

        skScalar sval = (skScalar)m_textures.size();
        sval          = skSqrt(sval);

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

    void displayGrid()
    {
        skScalar xMin, xMax, yMin, yMax, val, step, vStp, vMin, vMax;
        DrawUtils::SetColor(m_renderer, BackgroundGraph2);

        bool doGrid = skEqT(m_xForm.getZoom(), 1, .5);
        val         = (doGrid ? m_mapCell : m_mapCellSq) / m_xForm.getZoom();

        xMin = 0;
        yMin = 0;
        xMax = m_xForm.viewportWidth();
        yMax = m_xForm.viewportHeight();

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

    void render()
    {
        DrawUtils::Clear(m_renderer, Background2);

        DrawUtils::SetViewport(m_renderer, m_xForm);

        skScalar x1, y1, x2, y2;

        Images::Iterator it = m_textures.iterator();
        while (it.hasMoreElements())
        {
            PixelMap* map = it.getNext();

            map->getPosition().getBounds(x1, y1, x2, y2);

            x1 = m_xForm.getViewX(x1), x2 = m_xForm.getViewX(x2);
            y1 = m_xForm.getViewY(y1), y2 = m_xForm.getViewY(y2);

            const SDL_Rect dest = {
                (int)x1,
                (int)y1,
                (int)(x2 - x1),
                (int)(y2 - y1),
            };

            SDL_RenderCopy(m_renderer,
                           map->getTexture(),
                           nullptr,
                           &dest);
        }

        if (m_showGrid)
            displayGrid();

        DrawUtils::SetViewport(m_renderer, m_size);
        setColor(skPalette::Grey05);
        DrawUtils::StrokeScreenRect(m_renderer, m_xForm.getViewport());

        SDL_RenderPresent(m_renderer);
    }

    void buildMaps()
    {
        m_textures.reserve(m_parent->m_images.size());

        const int modStep = (int)skSqrt((skScalar)m_parent->m_images.size() / 2);

        int         x = 0, y = 0;
        skRectangle workingRect(0, 0, m_mapCellSq, m_mapCellSq);

        for (SKuint32 i = 0; i < m_parent->m_images.size(); ++i)
        {
            skImage* image = m_parent->m_images[i];

            SK_ASSERT(image->getWidth() == m_parent->m_max);
            SK_ASSERT(image->getHeight() == m_parent->m_max);

            SDL_Texture* tex = SDL_CreateTexture(m_renderer,
                                                 SDL_PIXELFORMAT_ABGR8888,
                                                 SDL_TEXTUREACCESS_STREAMING,
                                                 m_parent->m_max,
                                                 m_parent->m_max);

            if (tex != nullptr)
            {
                void* pixels;
                int   pitch;
                SDL_LockTexture(tex, nullptr, &pixels, &pitch);
                if (pixels)
                {
                    skMemcpy(pixels, image->getBytes(), image->getSizeInBytes());
                    SDL_UnlockTexture(tex);

                    const skPixel p(LineColor);
                    SDL_SetTextureColorMod(tex, p.r, p.g, p.b);
                    SDL_SetTextureScaleMode(tex, SDL_ScaleModeNearest);
                    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

                    workingRect.x = skScalar(x) * m_mapCellSq;
                    workingRect.y = skScalar(y) * m_mapCellSq;

                    PixelMap* pixelMap = new PixelMap(tex, workingRect);
                    m_textures.push_back(pixelMap);
                }
            }

            if (y++ % modStep == modStep - 1)
            {
                y = 0;
                x++;
            }

            ++i;
            delete image;
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

        m_size.x    = skScalar(w);
        m_size.y    = skScalar(h);
        m_mapCell   = skScalar(m_parent->m_max);
        m_mapCellSq = m_mapCell * m_mapCell;

        m_font = new Font();
        m_font->setPointScale(10);
        m_font->loadInternal(m_renderer, 48, 96);

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
