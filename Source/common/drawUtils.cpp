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
#include "drawUtils.h"
#include <SDL_render.h>

void DrawUtils::Clear(SDL_Renderer*   renderer,
                      const skColori& color)
{
    Clear(renderer, skColor(color));
}

void DrawUtils::Clear(SDL_Renderer*  renderer,
                      const skColor& color)
{
    SKuint8 r, g, b, a;
    color.asInt8(r, g, b, a);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    SDL_RenderClear(renderer);
}

void DrawUtils::SetViewport(SDL_Renderer*            renderer,
                            const skScreenTransform& xForm)
{
    const SDL_Rect subRect = {
        (int)xForm.viewportLeft(),
        (int)xForm.viewportTop(),
        (int)xForm.viewportWidth(),
        (int)xForm.viewportHeight(),
    };
    SDL_RenderSetViewport(renderer, &subRect);
}

void DrawUtils::SetViewport(SDL_Renderer*    renderer,
                            const skVector2& size)
{
    const SDL_Rect subRect = {
        (int)0,
        (int)0,
        (int)size.x,
        (int)size.y,
    };
    SDL_RenderSetViewport(renderer, &subRect);
}

void DrawUtils::ScreenLineTo(SDL_Renderer*   renderer,
                             const skScalar& x1,
                             const skScalar& y1,
                             const skScalar& x2,
                             const skScalar& y2)
{
    SDL_RenderDrawLine(renderer, (int)x1, (int)y1, (int)x2, (int)y2);
}

void DrawUtils::LineTo(SDL_Renderer*            renderer,
                       const skScreenTransform& xForm,
                       const skScalar&          x1,
                       const skScalar&          y1,
                       const skScalar&          x2,
                       const skScalar&          y2)
{
    SDL_RenderDrawLine(renderer,
                       (int)xForm.getViewX(x1),
                       (int)xForm.getViewY(y1),
                       (int)xForm.getViewX(x2),
                       (int)xForm.getViewY(y2));
}

void DrawUtils::StrokeScreenRect(SDL_Renderer* renderer,
                                 const skRectangle&  rect)
{
    const SDL_Rect r = {
        (int)rect.x,
        (int)rect.y,
        (int)rect.width,
        (int)rect.height,
    };
    SDL_RenderDrawRect(renderer, &r);
}

void DrawUtils::StrokeRect(SDL_Renderer*            renderer,
                           const skScreenTransform& xForm,
                           const skRectangle&       rect)
{
    skScalar x0, y0, x1, y1;
    rect.getBounds(x0, y0, x1, y1);

    xForm.xToView(x0);
    xForm.xToView(x1);

    xForm.yToView(y0);
    xForm.yToView(y1);

    const SDL_Rect r = {
        (int)x0,
        (int)y0,
        (int)(x1 - x0),
        (int)(y1 - y0),
    };
    SDL_RenderDrawRect(renderer, &r);
}

void DrawUtils::FillScreenRect(SDL_Renderer*      renderer,
                               const skRectangle& rect)
{
    const SDL_Rect r = {
        (int)rect.x,
        (int)rect.y,
        (int)rect.width,
        (int)rect.height,
    };
    SDL_RenderFillRect(renderer, &r);
}

void DrawUtils::FillRect(SDL_Renderer*            renderer,
                         const skScreenTransform& xForm,
                         const skRectangle&       rect)
{
    skScalar x0, y0, x1, y1;
    rect.getBounds(x0, y0, x1, y1);

    xForm.xToView(x0);
    xForm.xToView(x1);

    xForm.yToView(y0);
    xForm.yToView(y1);

    const SDL_Rect r = {
        (int)x0,
        (int)y0,
        (int)(x1 - x0),
        (int)(y1 - y0),
    };
    SDL_RenderFillRect(renderer, &r);
}

void DrawUtils::SetColor(SDL_Renderer* renderer, const skColori& color)
{
    SKuint8 r, g, b, a;
    skColor c(color);
    c.asInt8(r, g, b, a);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

void DrawUtils::SetColor(SDL_Renderer* renderer, const skColor& color)
{
    SKuint8 r, g, b, a;
    color.asInt8(r, g, b, a);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}
