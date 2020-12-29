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
#include "Math/skScreenTransform.h"
#include "Math/skVector2.h"
#include "Utils/skLogger.h"
#include "drawUtils.h"
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
    SDL_Window*       m_window;
    SDL_Renderer*     m_renderer;
    Font*             m_font;
    FreqApplication*  m_parent;
    bool              m_quit;
    bool              m_redraw;
    bool              m_showGrid;
    bool              m_leftIsDown;
    bool              m_ctrlDown;
    skVector2         m_winSize;
    skRectangle       m_viewport;
    skScalar          m_xAxisScale;
    skScalar          m_yAxisScale;
    skVector2         m_displayOffs;
    skScreenTransform m_xForm;

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

    void fillGrid() const
    {
        skScalar xMin, xMax, yMin, yMax, val, step, vStp, vMin, vMax;
        xMin = 6;
        yMin = 6;
        xMax = m_winSize.x - 12;
        yMax = m_winSize.y - 12;

        DrawUtils::SetColor(m_renderer, BackgroundGraph2);

        val = 128.f / m_xForm.getZoom();

        step = yMin - skFmod(yMin - m_xForm.yOffs(), val);
        while (step < yMax)
        {
            vMin = xMin;
            vMax = xMax;
            vStp = step;

            DrawUtils::ScreenLineTo(m_renderer,
                                    vMin,
                                    vStp,
                                    vMax,
                                    vStp);
            step += val;
        }

        step = xMin - skFmod(xMin - m_xForm.xOffs(), val);
        while (step < xMax)
        {
            vMin = yMin;
            vMax = yMax;
            vStp = step;
            DrawUtils::ScreenLineTo(m_renderer,
                                    vStp,
                                    vMin,
                                    vStp,
                                    vMax);
            step += val;
        }
    }

    void fillLabels() const
    {
        const skRectangle left(0, 0, m_displayOffs.x, m_winSize.y);
        const skRectangle bottom(m_displayOffs.x,
                                 m_winSize.y - m_displayOffs.y,
                                 m_winSize.x - m_displayOffs.x,
                                 m_displayOffs.y);

        DrawUtils::SetColor(m_renderer, Background2);
        DrawUtils::FillScreenRect(m_renderer,
                                  left);

        DrawUtils::FillScreenRect(m_renderer,
                                  bottom);
        m_font->setPointScale(12);

        DrawUtils::SetColor(m_renderer, Clear);
        skScalar xMin, xMax, yMin, yMax, val, step, vStp, xFac, yFac;

        left.getBounds(xMin, yMin, xMax, yMax);
        // leave the space at the bottom
        yMax -= m_displayOffs.y;
        xMin += 6;
        xMax -= 6;

        val = 128.f / m_xForm.getZoom();

        yFac = 1.f / m_yAxisScale;
        xFac = 1.f / m_xAxisScale;

        step = yMin - skFmod(yMin - m_xForm.yOffs(), val);
        while (step < yMax)
        {
            vStp = step;

            // Get the position on the window
            skScalar value = -m_xForm.getScreenY(vStp) + m_displayOffs.y;

            // convert to a unit of freq [0 - range]
            value *= yFac;

            m_font->draw(m_renderer,
                         (int)value,
                         3.f,
                         vStp - 12,
                         Text);
            step += val;
        }

        bottom.getBounds(xMin, yMin, xMax, yMax);
        yMin += 2;
        yMax -= 2;

        step = xMin - skFmod(xMin - (m_displayOffs.x + m_xForm.xOffs()), val);
        while (step < xMax)
        {
            vStp = step;
            if (vStp > xMin)
            {
                // Position in the window minus the offset from the left
                skScalar value = m_xForm.getScreenX(vStp) - m_displayOffs.x;

                // convert to a unit of x [0 - 255]
                value *= xFac;

                m_font->draw(m_renderer,
                             (int)value,
                             vStp - 3,
                             m_winSize.y - m_displayOffs.y + 5,
                             Text);
            }
            step += val;
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
                    m_xForm.reset();
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
                m_xForm.zoom(120, evt.wheel.y > 0);
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
                        m_xForm.pan((skScalar)evt.motion.xrel,
                                    (skScalar)evt.motion.yrel);
                    }

                    m_redraw = true;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (evt.button.button == SDL_BUTTON_LEFT)
                {
                    m_leftIsDown = true;
                    SDL_CaptureMouse(SDL_TRUE);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (evt.button.button == SDL_BUTTON_LEFT)
                {
                    m_leftIsDown = false;
                    SDL_CaptureMouse(SDL_FALSE);
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

    void render() const
    {
        DrawUtils::Clear(m_renderer, Background);

        const skRectangle& vp = m_xForm.getViewport();

        SDL_Rect rVp = {
            (int)vp.x,
            (int)vp.y,
            (int)vp.width,
            (int)vp.height,
        };
        SDL_RenderSetViewport(m_renderer, &rVp);

        if (m_showGrid)
            fillGrid();

        DrawUtils::SetColor(m_renderer, LineColor);

        skVector2 f, t;
        skScalar  x = 0;
        for (SKsize i = 0; i < 256; i++)
        {
            const skScalar y = skScalar(m_parent->m_freqBuffer[i]) * m_yAxisScale;
            if (i == 0)
            {
                f.x = x;
                f.y = -y;
            }
            else
            {
                t.x = x;
                t.y = -y;

                DrawUtils::LineTo(m_renderer,
                                  m_xForm,
                                  f.x,
                                  f.y,
                                  t.x,
                                  t.y);
                f = t;
            }
            x += m_xAxisScale;
        }

        rVp = {
            0,
            0,
            (int)m_winSize.x,
            (int)m_winSize.y,
        };
        SDL_RenderSetViewport(m_renderer, &rVp);

        fillLabels();
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

        m_winSize.x = skScalar(w);
        m_winSize.y = skScalar(h);

        m_font = new Font();
        m_font->loadInternal(m_renderer, 32, 72);

        m_viewport = skRectangle(m_displayOffs.x, 0, m_winSize.x, m_winSize.y - m_displayOffs.y);

        m_xForm.setScaleLimit(1, 2000);
        m_xForm.setViewport(m_viewport);
        m_xForm.setInitialOrigin(0, m_viewport.getBottom());
        m_xForm.reset();

        m_yAxisScale = (m_viewport.height - m_displayOffs.y) / skScalar(m_parent->m_max);
        m_xAxisScale = (m_viewport.width - m_displayOffs.x) / skScalar(256.0);

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

void FreqApplication::main(SKint32 w, SKint32 h)
{
    PrivateApp app(this);
    app.run(w, h);
}
