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

enum SwitchIds
{
    HP_MARK = 0,
    HP_NOCOLOR,
    HP_FLAGS,
    HP_RANGE,
    HP_CSV,
    HP_MAX
};

const Switch Switches[HP_MAX] = {
    {
        HP_MARK,
        'm',
        "mark",
        "Mark a specific hexadecimal sequence.\n"
        "  - Base 16 [00-FFFFFFFF]",
        true,
        1,
    },
    {
        HP_NOCOLOR,
        0,
        "no-color",
        "Remove color output.",
        true,
        0,
    },
    {
        HP_FLAGS,
        'f',
        "flags",
        "Specify the print flags. 01|02|04|08|10\n"
        "  - Where the value is a bit flag of one or more of the following hex values.\n"
        "    - COLORIZE:           01\n"
        "    - SHOW_HEX:           02\n"
        "    - SHOW_ASCII:         04\n"
        "    - SHOW_ADDRESS:       08\n"
        "    - SHOW_FULL_ADDRRESS: 10",
        true,
        1,
    },
    {
        HP_RANGE,
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
        HP_CSV,
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
        m_code(-1),
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
        if (psr.parse(argc, argv, Switches, HP_MAX) < 0)
            return 1;

        if (psr.isPresent(HP_FLAGS))
            m_flags = psr.getValueInt(HP_FLAGS, 0, PF_DEFAULT | PF_FULLADDR, 10);

        if (psr.isPresent(HP_MARK))
            m_code = psr.getValueInt64(HP_MARK, 0, SK_NPOS, 16);

        if (psr.isPresent(HP_NOCOLOR))
            m_flags &= ~PF_COLORIZE;

        m_csv = psr.isPresent(HP_CSV);
        if (psr.isPresent(HP_RANGE))
        {
            m_addressRange[0] = psr.getValueInt(HP_RANGE, 0, SK_NPOS32, 16);
            m_addressRange[1] = psr.getValueInt(HP_RANGE, 1, SK_NPOS32, 10);
        }

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
