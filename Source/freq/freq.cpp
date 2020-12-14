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
#include "Utils/skMap.h"
#include "Utils/skMemoryStream.h"
#include "Utils/skMemoryUtils.h"
#include "Utils/skPlatformHeaders.h"
#include "Utils/skString.h"

using namespace skHexPrint;
using namespace skCommandLine;

const skCommandLine::Switch Switches[] = {
    {
        "Range",
        'r',
        "range",
        "Specify a start address and a range."
        " The input is in base 16.",
        true,
        2,
    },
    {
        "Drop",
        0,
        "no-drop",
        "Do not drop values with no occurrence.",
        true,
        0,
    },
    {
        "CSV",
        0,
        "csv",
        "Output a comma separated value sheet",
        true,
        0,
    },
};

class Application
{
private:
    skFileStream m_stream;
    SKuint32     m_addressRange[2];
    bool         m_includeZero;
    bool         m_csv;

    SKuint64 m_freqBuffer[256];

public:
    Application() :
        m_addressRange(),
        m_includeZero(false),
        m_freqBuffer()
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
        psr.initializeSwitches(Switches, sizeof(Switches) / sizeof(Switches[0]));

        if (psr.parse(argc, argv) < 0)
            return 1;

        if (psr.isPresent("r"))
        {
            ParseOption *op   = psr.getOption("r");
            m_addressRange[0] = op->getInt(0, 16);
            m_addressRange[1] = op->getInt(1, 10);
        }

        m_includeZero = psr.isPresent("no-drop");
        m_csv         = psr.isPresent("csv");

        using List = skCommandLine::Parser::List;
        List &args = psr.getArgList();
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
        skString tstr;

        SKsize n;
        SKsize a, r;
        n = m_stream.size();
        a = skClamp<SKsize>(m_addressRange[0], 0, n);
        r = skClamp<SKsize>(m_addressRange[1], 0, n);

        if (m_addressRange[0] != -1)
            m_stream.seek(a, SEEK_SET);
        else
        {
            a = 0;
            r = n;
        }

        SKuint64 addr = 0;

        SKsize br, tr = 0, i, m = 0;
        while (!m_stream.eof() && tr < r)
        {
            br = m_stream.read(buffer, skMin<SKsize>(1024, r));
            if (br != -1 && br > 0)
            {
                buffer[br] = 0;
                for (i = 0; i < br; ++i)
                {
                    unsigned char ch = (unsigned char)buffer[i];
                    m_freqBuffer[ch]++;
                }
            }
            tr += br;
        }

        for (i = 0; i < 256; ++i)
        {
            int v = (int)m_freqBuffer[i];
            if (v != 0 || m_includeZero)
                printf("%d,%i,\n", (int)i, v);
        }
        return 0;
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
