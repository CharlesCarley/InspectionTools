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
#include "Utils/CommandLine/skCommandLineParser.h"
#include "Utils/skFileStream.h"
#include "Utils/skHexPrint.h"
#include "Utils/skLogger.h"
#include "Utils/skMemoryUtils.h"
#include "Utils/skPlatformHeaders.h"
#include "Utils/skString.h"

using namespace skHexPrint;
using namespace skCommandLine;

enum SwitchIds
{
    FP_RANGE = 0,
    FP_NO_DROP_ZERO,
    FP_NO_COLOR,
    FP_TEXT_GRAPH,
    FP_MAX
};

const Switch Switches[FP_MAX] = {
    {
        FP_RANGE,
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
        FP_NO_DROP_ZERO,
        0,
        "no-drop",
        "Do not drop zero values.",
        true,
        0,
    },
    {
        FP_NO_COLOR,
        0,
        "no-color",
        "Disable color printing.",
        true,
        0,
    },
    {
        FP_TEXT_GRAPH,
        'g',
        "graph",
        "Display a text based bar graph.\n"
        "  - Arguments: [width, height]\n"
        "    - Width  [1 - 128]\n"
        "    - Height [10 - 256]\n",
        true,
        2,
    },
};

class Application
{
private:
    skFileStream m_stream;
    SKuint32     m_addressRange[2];
    bool         m_includeZero;
    SKuint64     m_freqBuffer[256];
    SKuint64     m_max;
    SKint32      m_width;
    SKint32      m_height;
    bool         m_csv;
    bool         m_color;

public:
    Application() :
        m_addressRange(),
        m_includeZero(false),
        m_freqBuffer(),
        m_max(0),
        m_width(64),
        m_height(16),
        m_csv(true),
        m_color(true)
    {
        m_addressRange[0] = SK_NPOS32;
        m_addressRange[1] = SK_NPOS32;

        skMemset(m_freqBuffer, 0, sizeof(SKuint64) * 256);
    }

    ~Application()
    {
    }

    int parse(int argc, char **argv)
    {
        Parser psr;
        if (psr.parse(argc, argv, Switches, FP_MAX) < 0)
            return 1;

        if (psr.isPresent(FP_RANGE))
        {
            m_addressRange[0] = psr.getValueInt(FP_RANGE, 0, SK_NPOS32, 16);
            m_addressRange[1] = psr.getValueInt(FP_RANGE, 1, SK_NPOS32, 10);
        }

        if (psr.isPresent(FP_TEXT_GRAPH))
        {
            m_csv = false;

            m_width  = psr.getValueInt(FP_TEXT_GRAPH, 0, 64);
            m_height = psr.getValueInt(FP_TEXT_GRAPH, 1, 16);

            m_width  = skClamp(m_width, 1, 128);
            m_height = skClamp(m_height, 10, 256);
        }

        m_color       = !psr.isPresent(FP_NO_COLOR);
        m_includeZero = psr.isPresent(FP_NO_DROP_ZERO);

        using StringArray = Parser::StringArray;
        StringArray &args = psr.getArgList();
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

    int print()
    {
        SKuint8 buffer[1025];

        SKsize n;
        SKsize a, r;
        n = m_stream.size();
        a = skClamp<SKsize>(m_addressRange[0], 0, n);
        r = skClamp<SKsize>(m_addressRange[1], 0, n);

        if (m_addressRange[0] != SK_NPOS32)
            m_stream.seek(a, SEEK_SET);
        else
            r = n;

        SKsize br, tr = 0, i;
        while (!m_stream.eof() && tr < r)
        {
            br = m_stream.read(buffer, skMin<SKsize>(1024, r));
            if (br != SK_NPOS32 && br > 0)
            {
                buffer[br] = 0;
                for (i = 0; i < br; ++i)
                {
                    const auto ch = (unsigned char)buffer[i];
                    if (ch != 0 || m_includeZero)
                        m_freqBuffer[ch]++;
                }
            }
            tr += br;
        }

        for (i = 0; i < 256; ++i)
        {
            if (m_max < m_freqBuffer[i])
                m_max = m_freqBuffer[i];
        }

        if (m_csv)
            printCSV();
        else
            printGraph();

        return 0;
    }

    void printGraph()
    {
        double codes[4] = {
            m_height * 0.2,
            m_height * 0.4,
            m_height * 0.6,
            m_height * 0.8,
        };

        bool    done = false;
        SKint32 y = 0, x = 0, i = 0, j = 0;
        SKint32 maxLeft = countPlaces(m_max) + 3;

        while (i < 256)
        {
            for (y = 0; y < m_height; ++y)
            {
                if (y == 0)
                {
                    j = i + m_width;
                    if (j > 255)
                        j -= (j - 255);

                    printf("\tBytes [%02X - %02X]\n", i, j);
                    for (j = 0; j < maxLeft; ++j)
                        putchar(' ');
                    putchar('+');
                    for (x = 0; x < m_width; ++x)
                        putchar('-');
                    putchar('\n');
                }

                const SKint32 ypos = (m_height - y);

                if (ypos % 4 == 0)
                {
                    double d = (double)m_max / ((double)y + 1);

                    SKint32 cw = printf("%0.2f", d);
                    SKint32 k  = (int)maxLeft - cw;

                    for (j = 0; j < k; ++j)
                        putchar(' ');
                }
                else
                {
                    for (j = 0; j < maxLeft; ++j)
                        putchar(' ');
                }
                putchar('|');

                j = i;
                for (x = 0; x < m_width && j < 256; ++x, ++j)
                {
                    auto val = (double)m_freqBuffer[j];
                    val /= (double)m_max;
                    val *= (double)m_height;

                    if (val > ypos)
                    {
                        char cc;
                        if (ypos < codes[0])
                            cc = '@';
                        else if (ypos < codes[1])
                            cc = '+';
                        else if (ypos < codes[2])
                            cc = '^';
                        else if (ypos < codes[3])
                            cc = ':';
                        else
                            cc = '.';

                        if (m_color)
                        {
                            if (cc == '@')
                                skDebugger::writeColor(skConsoleColorSpace::CS_DARKYELLOW);
                            else
                                skDebugger::writeColor(skConsoleColorSpace::CS_YELLOW);
                        }
                        putchar(cc);
                    }
                    else
                    {
                        if (m_color)
                            skDebugger::writeColor(CS_WHITE);
                        putchar(' ');
                    }
                }

                if (m_color)
                    skDebugger::writeColor(CS_WHITE);
                putchar('\n');
            }
            for (j = 0; j < maxLeft; ++j)
                putchar(' ');
            putchar('+');
            for (x = 0; x < m_width; ++x)
                putchar('-');
            putchar('\n');

            for (j = 0; j < maxLeft; ++j)
                putchar(' ');

            j = i;
            for (x = 0; x < m_width && j < 256; ++x, ++j)
            {
                if (x % 4 == 0)
                    printf(" %02X ", j);
            }
            putchar('\n');
            putchar('\n');
            i += m_width;
        }
        putchar('\n');
    }

    SKint32 countPlaces(SKint32 n)
    {
        SKint32 i = 0;
        while (n > 0)
        {
            ++i;
            n /= 10;
        }
        return i;
    }

    void printCSV()
    {
        for (SKint32 i = 0; i < 256; ++i)
        {
            const int v = (int)m_freqBuffer[i];
            if (v != 0 || m_includeZero)
                printf("%d, %d,\n", i, v);
        }
    }
};

int main(int argc, char **argv)
{
    skLogger log;
    log.setFlags(LF_STDOUT);
    log.setDetail(LD_VERBOSE);

    Application freq;
    if (freq.parse(argc, argv))
        return 1;
    return freq.print();
}
