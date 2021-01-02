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
const skColor  Text             = skColor(0xD5D5D5FF);
const SKuint32 WindowFlags      = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;

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
    bool              m_rightIsDown;
    bool              m_ctrlDown;
    skVector2         m_size;
    skVector2         m_clickPos;
    skVector2         m_mouseCo;
    skScalar          m_mapCell;
    skScalar          m_mapCellSq;
    skScreenTransform m_xForm;
    int               m_maxCellX;
    int               m_maxCellY;

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
        m_rightIsDown(false),
        m_ctrlDown(false),
        m_mapCell(1),
        m_mapCellSq(1),
        m_maxCellX(0),
        m_maxCellY(0)
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

        vp.width  = m_size.x;
        vp.height = m_size.y;

        skMath::forceAlign(vp.width, 16);
        skMath::forceAlign(vp.height, 16);

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
        SDL_SetRenderDrawColor(m_renderer,
                               color.r,
                               color.g,
                               color.b,
                               color.a);
    }

    void handleKeyDown(const SDL_Event& evt)
    {
        switch (evt.key.keysym.sym)
        {
        case SDLK_LCTRL:
            m_ctrlDown = true;
            break;
        case SDLK_ESCAPE:
            m_quit = true;
            break;
        case SDLK_KP_MINUS:
            m_xForm.zoom(120, false);
            m_redraw = true;
            break;
        case SDLK_KP_PLUS:
            m_xForm.zoom(120, true);
            m_redraw = true;
            break;
        case SDLK_c:
        case SDLK_KP_PERIOD:
            m_xForm.reset();
            m_redraw = true;
            break;
        case SDLK_g:
            m_showGrid = !m_showGrid;
            m_redraw   = true;
            break;
        default:
            break;
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
                handleKeyDown(evt);
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
            {
                if (m_leftIsDown)
                {
                    if (m_ctrlDown)
                    {
                        skVector2 dv(skScalar(12 * evt.motion.xrel),
                                     skScalar(12 * evt.motion.yrel));
                        m_xForm.zoom(dv.length(), dv.y < 0);
                    }
                    else
                        m_xForm.pan((skScalar)evt.motion.xrel, (skScalar)evt.motion.yrel);
                    m_redraw = true;
                }
                m_redraw    = true;
                m_mouseCo.x = skScalar(evt.motion.x);
                m_mouseCo.y = skScalar(evt.motion.y);
            }
            break;
            case SDL_MOUSEBUTTONDOWN:
                if (evt.button.button == SDL_BUTTON_LEFT)
                {
                    SDL_CaptureMouse(SDL_TRUE);
                    m_leftIsDown = true;
                    m_redraw     = true;
                }
                else if (evt.button.button == SDL_BUTTON_RIGHT)
                {
                    m_clickPos.x  = skScalar(evt.button.x);
                    m_clickPos.y  = skScalar(evt.button.y);
                    m_rightIsDown = true;
                    m_redraw      = true;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (evt.button.button == SDL_BUTTON_LEFT)
                {
                    SDL_CaptureMouse(SDL_FALSE);
                    m_leftIsDown = false;
                }
                else if (evt.button.button == SDL_BUTTON_RIGHT)
                {
                    m_rightIsDown = false;
                    m_redraw      = true;
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

        const bool doGrid = skEqT(m_xForm.getZoom(), 1, .5);

        val = (doGrid ? m_mapCell : m_mapCellSq) / m_xForm.getZoom();

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

    void splitMappedMouseCo(skScalar& mapCo,
                            int&      arrayIndex,
                            int&      mapIndex,
                            const int maxArrayIndex) const
    {
        skScalar wp, fp;
        mapCo /= m_mapCellSq / m_xForm.getZoom();

        if (mapCo >= 0)
        {
            skSplitScalar(mapCo, wp, fp);
            arrayIndex = (int)wp;
            if (arrayIndex < maxArrayIndex + 1)
                mapIndex = (int)skFloor(fp * m_mapCell);
            else
                arrayIndex = -1;
        }
        else
            arrayIndex = -1;
    }

    void getMouseCoMappedToCellXY(int& xArray, int& yArray, int& xMap, int& yMap) const
    {
        // Grab the mouse coordinate relative to the viewport
        // then take off the current offset and divide it as
        // a ratio of the cell to find its major index.
        // Then take the fractional part (its ratio of one cell)
        // to get its index into the map itself.
        // Be sure to test the index  (xArray * m_maxCellX + yArray)

        skScalar mapCo;
        mapCo = m_mouseCo.x - m_xForm.viewportLeft();
        mapCo -= m_xForm.xOffs();
        splitMappedMouseCo(mapCo, xArray, xMap, m_maxCellX);

        mapCo = m_mouseCo.y - m_xForm.viewportTop();
        mapCo -= m_xForm.yOffs();
        splitMappedMouseCo(mapCo, yArray, yMap, m_maxCellY);

        if (xArray == -1 || yArray == -1)
            xArray = xMap = yArray = yMap = -1;
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

        int xArray, yArray;
        int xMap, yMap;
        getMouseCoMappedToCellXY(xArray, yArray, xMap, yMap);

        m_font->draw(m_renderer, "X", 20, 20, Text);
        m_font->draw(m_renderer, xArray, 35, 20, Text);
        m_font->draw(m_renderer, "Y", 20, 40, Text);
        m_font->draw(m_renderer, yArray, 35, 40, Text);
        m_font->draw(m_renderer, "u", 20, 60, Text);
        m_font->draw(m_renderer, xMap, 35, 60, Text);
        m_font->draw(m_renderer, "v", 20, 80, Text);
        m_font->draw(m_renderer, yMap, 35, 80, Text);

        char buf[32] = {};

        if (xArray >= 0)
        {
            SKuint32 i1 = (SKuint32)xArray * (SKuint32)m_maxCellX + (SKuint32)yArray;

            if (i1 < m_textures.size())
            {
                const SKubyte b  = m_textures.at(i1)->getByte(xMap, yMap);
                SKbyte        cc = ' ';

                if (b >= 32 && b < 127)
                    cc = b;

                i1 = (SKuint32)xArray * (SKuint32)m_maxCellX + (SKuint32)yArray;

                SKuint32 address = (SKuint32)(yMap * SKuint32(m_mapCell)) + (SKuint32)xMap;
                address += SKuint32(m_mapCellSq) * i1;

                int len = skSprintf(buf, 31, "0x%08X", address);
                buf[len] = 0;
                m_font->draw(m_renderer, buf, 20, 100, Text);

                len = skSprintf(buf, 31, "(0x%02X, '%c', %d)", b, cc, (int)b);
                buf[len] = 0;
                m_font->draw(m_renderer, buf, 20, 120, Text);
            }
        }

        DrawUtils::SetViewport(m_renderer, m_size);

        setColor(skPalette::Grey05);
        DrawUtils::StrokeScreenRect(m_renderer, m_xForm.getViewport());
        SDL_RenderPresent(m_renderer);
    }

    void buildMaps()
    {
        m_textures.reserve(m_parent->m_images.size());

        const int modStep = (int)skSqrt((skScalar)m_parent->m_images.size());

        m_maxCellY = modStep - 1;
        m_maxCellX = 0;
        int y      = 0;

        skRectangle workingRect(0, 0, m_mapCellSq, m_mapCellSq);

        for (SKuint32 i = 0; i < m_parent->m_images.size(); ++i)
        {
            skImage* image = m_parent->m_images[i];

            SK_ASSERT(image->getWidth() == m_parent->m_max);
            SK_ASSERT(image->getHeight() == m_parent->m_max);

            workingRect.x = skScalar(m_maxCellX) * m_mapCellSq;
            workingRect.y = skScalar(y) * m_mapCellSq;

            PixelMap* px = new PixelMap(image, workingRect);

            px->loadFromImage(m_renderer, skPixel(LineColor.asInt()));

            m_textures.push_back(px);

            if (++y % modStep == 0)
            {
                y = 0;
                m_maxCellX++;
            }
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
                                        WindowFlags);
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
        m_font->loadInternal(m_renderer, 48, 72);
        m_font->setPointScale(12);

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
