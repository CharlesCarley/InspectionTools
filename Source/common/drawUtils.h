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
#ifndef _drawUtils_h_
#define _drawUtils_h_

#include "Math/skColor.h"
#include "Math/skScreenTransform.h"

struct SDL_Renderer;

namespace DrawUtils
{
    extern void Clear(SDL_Renderer*   renderer,
                      const skColori& color);

    extern void Clear(SDL_Renderer*  renderer,
                      const skColor& color);

    extern void SetViewport(SDL_Renderer*            renderer,
                            const skScreenTransform& xForm);
    extern void SetViewport(SDL_Renderer*    renderer,
                            const skVector2& size);

    extern void ScreenLineTo(SDL_Renderer*   renderer,
                             const skScalar& x1,
                             const skScalar& y1,
                             const skScalar& x2,
                             const skScalar& y2);

    extern void LineTo(SDL_Renderer*            renderer,
                       const skScreenTransform& xForm,
                       const skScalar&          x1,
                       const skScalar&          y1,
                       const skScalar&          x2,
                       const skScalar&          y2);

    extern void StrokeScreenRect(SDL_Renderer* renderer,
                                 const skRectangle&  rect);

    extern void StrokeRect(SDL_Renderer*            renderer,
                           const skScreenTransform& xForm,
                           const skRectangle&       rect);

    extern void FillScreenRect(SDL_Renderer*      renderer,
                               const skRectangle& rect);

    extern void FillRect(SDL_Renderer*            renderer,
                         const skScreenTransform& xForm,
                         const skRectangle&       rect);

    extern void SetColor(SDL_Renderer* renderer, const skColori& color);

    extern void SetColor(SDL_Renderer* renderer, const skColor& color);
};  // namespace DrawUtils

#endif  //_drawUtils_h_
