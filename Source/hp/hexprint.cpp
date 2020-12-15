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
#include "Utils/skString.h"

using namespace skHexPrint;
using namespace skCommandLine;

const Switch Switches[] = {
    {
        "mark",
        'm',
        "mark",
        "Mark a specific hexadecimal sequence."
        " Input is in base 16.",
        true,
        1,
    },
    {
        "NoColor",
        0,
        "no-color",
        "Remove color output.",
        true,
        0,
    },
    {
        "Flags",
        'f',
        "flags",
        "Specify the print flags. 1|2|4|8|16",
        true,
        1,
    },
    {
        "Base",
        'b',
        "base",
        "Specify the base for the print out."
        " The input is in base 16.",
        true,
        1,
    },
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
        "CSV",
        0,
        "csv",
        "Converts the output to a comma separated buffer",
        true,
        0,
    },
};

class Application
{
private:
    skFileStream m_stream;
    SKint64      m_code;
    SKuint32     m_addressRange[2];
    SKuint32     m_flags;
    bool         m_csv;

public:
    Application() :
        m_code(0),
        m_addressRange(),
        m_flags(PF_DEFAULT | PF_FULLADDR),
        m_csv(false)
    {
        m_addressRange[0] = SK_NPOS32;
        m_addressRange[1] = SK_NPOS32;
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

        if (psr.isPresent("flags"))
        {
            ParseOption *op = psr.getOption("flags");
            m_flags         = op->getInt();
        }

        if (psr.isPresent("mark"))
        {
            ParseOption *op = psr.getOption("mark");
            m_code          = op->getInt64(0, 16);
        }

        if (psr.isPresent("no-color"))
            m_flags &= ~PF_COLORIZE;

        m_csv = psr.isPresent("csv");

        if (psr.isPresent("r"))
        {
            ParseOption *op   = psr.getOption("r");
            m_addressRange[0] = op->getInt(0, 16);
            m_addressRange[1] = op->getInt(1, 10);
        }

        using List = Parser::List;
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

    void dumpCSV(const void * ptr,
                 const SKsize offset,
                 const SKsize len)
    {
        if (!ptr || offset == SK_NPOS || len == SK_NPOS)
            return;
        SKsize      i, j;
        const char *cp = (const char *)ptr;
        for (i = 0; i < len; i += 16)
        {
            for (j = 0; j < 16; j++)
            {
                const auto c = (unsigned char)cp[i + j];
                printf("0x%02X, ", c);
            }
            printf("\n");
        }
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
        {
            a = 0;
            r = n;
        }

        SKsize br, tr = 0;
        while (!m_stream.eof() && tr < r)
        {
            br = m_stream.read(buffer, skMin<SKsize>(1024, r));
            if (br != SK_NPOS32 && br > 0)
            {
                buffer[br] = 0;
                if (m_csv)
                    dumpCSV(buffer, tr + a, br);
                else
                    dumpHex(buffer, tr + a, br, m_flags, m_code);
                tr += br;
            }
        }
        return 0;
    }
};

int main(int argc, char **argv)
{
    skLogger log;
    log.setFlags(LF_STDOUT);
    log.setDetail(LD_VERBOSE);

    Application sp;
    if (sp.parse(argc, argv))
        return 1;
    return sp.print();
}
