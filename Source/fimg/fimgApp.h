/*
-------------------------------------------------------------------------------
  Copyright (c) Charles Carley.

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
#ifndef _freqApp_h_
#define _freqApp_h_

#include "Utils/skString.h"
#include "Utils/skArray.h"
#include "Image/skImage.h"



class FimgApplication
{
public:

    typedef skArray<skImage*> Images;




protected:


    Images m_images;
    SKuint64 m_max;

    friend class PrivateApp;

    void clear();

public:
    FimgApplication() :
        m_max(0)
    {
    }

    virtual ~FimgApplication()
    {
        clear();
    }


    void run(SKint32 w, SKint32 h);
};

#endif  //_freqApp_h_
