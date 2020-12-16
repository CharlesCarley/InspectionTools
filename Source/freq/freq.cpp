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
    FP_TEXT_GRAPH,
    FP_MAX
};

const Switch Switches[FP_MAX] = {
    {
        FP_RANGE,
        'r',
        "range",
        "Specify a start address and a range.\n"
        "The input is in base 16.",
        true,
        2,
    },
    {
        FP_NO_DROP_ZERO,
        0,
        "no-drop",
        "Do not drop values with zero occurrences.",
        true,
        0,
    },
    {
        FP_TEXT_GRAPH,
        'g',
        "graph",
        "Display a text based bar graph.\n"
        "Arguments [rows, characters per row]",
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
    SKsize       m_rows;
    SKsize       m_charsPerRow;
    bool         m_csv;

public:
    Application() :
        m_addressRange(),
        m_includeZero(false),
        m_freqBuffer(),
        m_rows(3),
        m_charsPerRow(10),
        m_csv(true)
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

            m_rows        = psr.getValueInt(FP_TEXT_GRAPH, 0, 3);
            m_charsPerRow = psr.getValueInt(FP_TEXT_GRAPH, 1, 10);

            m_rows        = skClamp<SKsize>(m_rows, 2, 8);
            m_charsPerRow = skClamp<SKsize>(m_charsPerRow, 5, 80);
        }

        m_includeZero = psr.isPresent(FP_NO_DROP_ZERO);

        using StringArray = Parser::StringArray;
        StringArray &args = psr.getArgList();
        if (args.empty())
        {
            skLogf(LD_ERROR, "No files supplied\n");
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
        SKuint8  buffer[1025];
        skString tmpStr;

        SKsize n;
        SKsize a, r;
        n = m_stream.size();
        a = skClamp<SKsize>(m_addressRange[0], 0, n);
        r = skClamp<SKsize>(m_addressRange[1], 0, n);

        if (m_addressRange[0] != SK_NPOS32)
            m_stream.seek(a, SEEK_SET);
        else
        {
            r = n;
        }

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

        if (m_csv)
            printCSV();
        else
            printGraph();

        return 0;
    }

    void printGraph()
    {
        SKsize max = 0, i, r;
        for (i = 0; i < 256; ++i)
            max = skMax<SKsize>(max, m_freqBuffer[i]);

        const SKsize SubDivision = m_rows;
        const SKsize Max         = 256;
        const SKsize PerCol      = m_charsPerRow;
        const SKsize MaxY        = (PerCol * SubDivision) + SubDivision;

        const SKsize NumCol = Max / SubDivision;

        double codes[4] = {
            PerCol * 0.2,
            PerCol * 0.4,
            PerCol * 0.6,
            PerCol * 0.8,
        };

        r = 0;
        SKsize y, x, s = 0;

        for (y = 0; y < MaxY; ++y)
        {
            skDebugger::writeColor(CS_WHITE, CS_BLACK);
            if (y > 0 && y % PerCol == 0)
            {
                printf("   +");
                for (x = 0; x < NumCol; ++x)
                {
                    printf("-");
                }

                ++r;
                if (r >= m_rows)
                    break;

                s = 0;
            }
            else
            {
                const SKsize bufferOffs = r * NumCol;
                if (s == 0)
                {
                    skDebugger::writeColor(CS_LIGHT_GREY);
                    printf("\n    Bytes: [%i-%i]\n", (int)bufferOffs, (int)(bufferOffs + NumCol));
                }

                skDebugger::writeColor(CS_GREY);
                if (s % 3 == 0)
                    printf("%3i ", (int)s);
                else
                    printf("    ");

                char       cc;
                const auto step = (double)(PerCol - s++ % PerCol);
                if (step < codes[0])
                    cc = '@';
                else if (step < codes[1])
                    cc = '+';
                else if (step < codes[2])
                    cc = '^';
                else if (step < codes[3])
                    cc = ':';
                else
                    cc = '.';
                const auto yoffs = (double)(PerCol * (r + 1) - (y + 1));

                for (x = 0; x < NumCol; ++x)
                {
                    i = x + bufferOffs;
                    if (i >= 256)
                        break;

                    auto val = (double)m_freqBuffer[i];
                    val /= (double)max;
                    val *= (double)PerCol;

                    if (val > yoffs)
                    {
                        skDebugger::writeColor(CS_YELLOW);
                        printf("%c", cc);
                    }
                    else
                    {
                        skDebugger::writeColor(CS_WHITE);
                        printf(" ");
                    }
                }
                skDebugger::writeColor(CS_WHITE);
                printf("\n");
            }
        }
        printf("\n");
    }

    void printCSV()
    {
        for (SKsize i = 0; i < 256; ++i)
        {
            const int v = (int)m_freqBuffer[i];
            if (v != 0 || m_includeZero)
                printf("%d,%i,\n", (int)i, v);
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
