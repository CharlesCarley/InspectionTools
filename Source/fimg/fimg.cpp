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
#include "Image/skImage.h"
#include "Math/skMath.h"
#include "Utils/CommandLine/skCommandLineParser.h"
#include "Utils/skFileStream.h"
#include "Utils/skHexPrint.h"
#include "Utils/skLogger.h"
#include "Utils/skPlatformHeaders.h"
#include "Utils/skString.h"
#include "fimgApp.h"

using namespace skHexPrint;
using namespace skCommandLine;

enum SwitchIds
{
    FI_RANGE = 0,
    FI_MAX,
    FI_OFFSET,
    FI_FLIP,
#ifdef USING_SDL
    FI_GRAPH,
#endif
    FI_OUTPUT,
    FI_MAX_ENUM
};

const Switch Switches[FI_MAX_ENUM] = {
    {
        FI_RANGE,
        'r',
        "range",
        "Specify a start address and a range.\n"
        "  - Arguments: [address, range]\n"
        "    - Address Base 16 [0 - file length]\n"
        "    - Range   Base 10 [0 - file length]\n",
        true,
        2,
    },
    {
        FI_MAX,
        'm',
        "maxX",
        "Specify pixel range that determines a new row.",
        true,
        1,
    },
    {
        FI_OFFSET,
        0,
        "offset",
        "Specify pixel range that determines a new row.",
        true,
        1,
    },
    {
        FI_FLIP,
        'f',
        nullptr,
        "Flip the x and y coordinates.",
        true,
        0,
    },
#ifdef USING_SDL
    {
        FI_GRAPH,
        'w',
        "window",
        "Graph items in a window.\n"
        "  - Arguments: [width, height]\n"
        "    - Width  [200 - 7680]\n"
        "    - Height [100 - 4320]\n",
        true,
        2,
    },
#endif
    {
        FI_OUTPUT,
        'o',
        "output",
        "Specify the output file.",
        true,
        1,
    },
};

class Application : public FimgApplication
{
private:
    skFileStream m_stream;
    SKuint32     m_addressRange[2];
    SKint32      m_width;
    SKint32      m_height;
    bool         m_window;
    skString     m_output;

public:
    Application() :
        m_addressRange(),
        m_width(800),
        m_height(600),
        m_window(false)
    {
        skImage::initialize();
        m_addressRange[0] = SK_NPOS32;
        m_addressRange[1] = SK_NPOS32;
    }

    virtual ~Application()
    {
        skImage::finalize();
    }

    int parse(int argc, char** argv)
    {
        Parser psr;
        if (psr.parse(argc, argv, Switches, FI_MAX_ENUM) < 0)
            return 1;

        if (psr.isPresent(FI_RANGE))
        {
            m_addressRange[0] = psr.getValueInt(FI_RANGE, 0, SK_NPOS32, 16);
            m_addressRange[1] = psr.getValueInt(FI_RANGE, 1, SK_NPOS32, 10);
        }

        m_max = skClamp<SKint32>(psr.getValueInt(FI_MAX, 0, 16), 16, 256);
        if (psr.isPresent(FI_GRAPH))
        {
            m_window = true;
            m_width  = psr.getValueInt(FI_GRAPH, 0, 640);
            m_height = psr.getValueInt(FI_GRAPH, 1, 480);

            m_width  = skClamp(m_width, 200, 7680);
            m_height = skClamp(m_height, 100, 4320);
        }

        m_output = psr.getValueString(FI_OUTPUT, 0);
        if (m_output.empty() && !m_window)
        {
            skLogf(LD_ERROR, "No output file supplied\n");
            return 1;
        }

        using StringArray = Parser::StringArray;
        StringArray& args = psr.getArgList();
        if (args.empty())
        {
            skLogf(LD_INFO, "No file supplied\n");
            return 1;
        }

        m_stream.open(args[0].c_str(), skStream::READ);
        if (!m_stream.isOpen())
        {
            skLogf(LD_ERROR, "Failed to open file %s\n", args[0].c_str());
            return 1;
        }

        return 0;
    }

    void buildImage()
    {
        SKuint8 buffer[1025];

        const SKsize n = m_stream.size();
        const SKsize a = skClamp<SKsize>(m_addressRange[0], 0, n);
        SKsize       r = skClamp<SKsize>(m_addressRange[1], 0, n);

        if (m_addressRange[0] != SK_NPOS32)
            m_stream.seek(a, SEEK_SET);
        else
            r = n;

        SKint32  x = 0, y = 0;
        skImage* working = new skImage(m_max, m_max, skPixelFormat::SK_RGBA);
        m_images.push_back(working);

        SKsize tr = 0;
        while (!m_stream.eof() && tr < r)
        {
            const SKsize br = m_stream.read(buffer, skMin<SKsize>(1024, r));
            if (br != SK_NPOS32 && br > 0)
            {
                buffer[br] = 0;

                if (br > 3)
                {
                    for (SKsize i = 0; i < br; ++i)
                    {
                        const auto ch = static_cast<SKubyte>(buffer[i]);

                        skPixel color(0x00000000);
                        color.set(skPixel(ch, ch, ch, 128));

                        if (x++ % m_max == m_max - 1)
                        {
                            x = 0;
                            if (y++ % m_max == m_max - 1)
                            {
                                y       = 0;
                                working = new skImage(m_max, m_max, skPixelFormat::SK_RGBA);
                                m_images.push_back(working);
                            }
                        }
                        working->setPixel(x, (m_max - 1) - y, color);
                    }
                }
            }
            tr += br;
        }
    }

    int print()
    {
        buildImage();
        run(m_width, m_height);
        return 0;
    }
};

int main(int argc, char** argv)
{
    skLogger log;
    log.setFlags(LF_STDOUT);
    log.setDetail(LD_VERBOSE);

    Application freq;
    if (freq.parse(argc, argv))
        return 1;
    return freq.print();
}
