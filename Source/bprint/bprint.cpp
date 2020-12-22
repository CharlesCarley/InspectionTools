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
#include <cmath>
#include "Utils/CommandLine/skCommandLineParser.h"
#include "Utils/skFileStream.h"
#include "Utils/skHexPrint.h"
#include "Utils/skLogger.h"
#include "Utils/skString.h"

using namespace skHexPrint;
using namespace skCommandLine;

enum SwitchIds
{
    BP_SHOW_ADDRESS = 0,
    BP_PAD_ZERO,
    BP_SYMBOLS,
    BP_BASE,
    BP_SHIFT,
    BP_RANGE,
    BP_ADD_WHITESPACE,
    BP_ADD_NEW_LINE,
    BP_OUT,
    BP_MAX
};

const Switch Switches[BP_MAX] = {
    {
        BP_SHOW_ADDRESS,
        0,
        "show-address",
        "Display the current address.",
        true,
        0,
    },
    {
        BP_PAD_ZERO,
        'p',
        "pad",
        "Supply a symbol string",
        true,
        0,
    },
    {
        BP_SYMBOLS,
        0,
        "symbols",
        "Supply a symbol string",
        true,
        1,
    },
    {
        BP_BASE,
        'b',
        "base",
        "convert base",
        true,
        1,
    },
    {
        BP_SHIFT,
        's',
        "shift",
        "convert base",
        true,
        1,
    },
    {
        BP_RANGE,
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
        BP_ADD_WHITESPACE,
        0,
        "ws",
        "Add white space",
        true,
        0,
    },
    {
        BP_ADD_NEW_LINE,
        0,
        "nl",
        "Add white space",
        true,
        1,
    },
    {
        BP_OUT,
        'o',
        "output",
        "Add white space",
        true,
        1,
    },
};

class Application
{
private:
    skFileStream m_stream;
    SKuint32     m_addressRange[2];
    skString     m_tmp;
    skString     m_symbols;
    SKint32      m_base;
    SKint32      m_nl;
    SKint32      m_total;
    SKint32      m_charsPerBase;
    SKint32      m_shift;
    bool         m_whitespace;
    bool         m_pad;

    void makeSymbolDefault()
    {
        m_symbols.reserve(63);  // +1
        int i;
        for (i = 0; i < 10; ++i)
            m_symbols.append('0' + i);
        for (i = 0; i < 26; ++i)
            m_symbols.append('A' + i);
        for (i = 0; i < 26; ++i)
            m_symbols.append('a' + i);
    }

public:
    Application() :
        m_addressRange(),
        m_base(10),
        m_nl(0),
        m_total(0),
        m_charsPerBase(0),
        m_shift(0),
        m_whitespace(false),
        m_pad(false)
    {
        m_addressRange[0] = SK_NPOS32;
        m_addressRange[1] = SK_NPOS32;

        makeSymbolDefault();
    }

    ~Application()
    {
    }

    int parse(int argc, char **argv)
    {
        Parser psr;
        if (psr.parse(argc, argv, Switches, BP_MAX) < 0)
            return 1;

        if (psr.isPresent(BP_RANGE))
        {
            m_addressRange[0] = psr.getValueInt(BP_RANGE, 0, SK_NPOS32, 16);
            m_addressRange[1] = psr.getValueInt(BP_RANGE, 1, SK_NPOS32, 10);
        }

        m_whitespace = psr.isPresent(BP_ADD_WHITESPACE);
        m_pad        = psr.isPresent(BP_PAD_ZERO);

        if (psr.isPresent(BP_ADD_NEW_LINE))
            m_nl = psr.getValueInt(BP_ADD_NEW_LINE, 0, 0);

        if (psr.isPresent(BP_SYMBOLS))
            m_symbols = psr.getValueString(BP_SYMBOLS, 0);

        if (psr.isPresent(BP_BASE))
        {
            m_base = psr.getValueInt(BP_BASE, 0, 10);

            if (m_base < 2)
            {
                skLogd(LD_ERROR, "base must be greater than 1\n");
                return 1;
            }

            m_charsPerBase = charsPerBase(m_base);
        }

        if (psr.isPresent(BP_SHIFT))
        {
            m_shift = psr.getValueInt(BP_SHIFT, 0, 0);
            m_shift = skMax(0, m_shift);
        }


        if (m_symbols.empty())
        {
            skLogd(LD_ERROR, "Symbol string must not be empty\n");
            return 1;
        }

        if (m_base > m_symbols.size())
        {
            skLogf(LD_ERROR, "The symbol string must contain at least %d symbols\n", m_base);
            return 1;
        }

        using StringArray = Parser::StringArray;
        StringArray &args = psr.getArgList();
        if (args.empty())
        {
            skLogi("No file supplied\n");
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

    int charsPerBase(int base)
    {
        int ln = int(ceil(log(255.0) / log((double)(base))));
        if (ln < 2)
            ln = 2;
        if (ln > 8)
            ln = 8;
        return ln;
    }

    int print()
    {
        SKuint8 buffer[1025];
        m_tmp.reserve(16);

        SKsize n;
        SKsize a, r;
        n = m_stream.size();
        a = skClamp<SKsize>(m_addressRange[0], 0, n);
        r = skClamp<SKsize>(m_addressRange[1], 0, n);

        if (m_addressRange[0] != SK_NPOS32)
            m_stream.seek(a, SEEK_SET);
        else
            r = n;

        m_total = 0;

        SKuint64 address = 0;
        SKsize   br, tr = 0, i = 0;
        while (!m_stream.eof() && tr < r)
        {
            br = m_stream.read(buffer, skMin<SKsize>(1024, r));
            if (br != SK_NPOS32 && br > 0)
            {
                buffer[br] = 0;
                for (i = 0; i < br; ++i)
                {
                    SKuint8 byte = buffer[i];
                    printBase(byte);

                    if (m_nl > 0)
                    {
                         if (m_total++ % (m_nl) == (m_nl - 1))
                             putchar('\n');
                    }
                }
            }
            tr += br;
        }
        putchar('\n');
        return 0;
    }

    void printBase(SKuint8 inp)
    {
        m_tmp.resize(0);
        int q, r;

        if (inp == 0)
        {
            r = 0;
            if (m_shift > 0)
            {
                r += m_shift;
                r %= m_base;
            }
            m_tmp.append(m_symbols.at(r));
        }
        else
        {
            while (inp > 0)
            {
                q = inp / m_base;
                r = inp % m_base;

                if (r < m_symbols.size())
                {
                    if (m_shift > 0)
                    {
                        r += m_shift;
                        r %= m_base;
                    }
                    m_tmp.append(m_symbols.at(r));
                }
                inp = q;
            }
        }

        if (m_pad)
        {
            int cpb = charsPerBase(m_base);
            while (m_tmp.size() < cpb)
            {
                r = 0;
                if (m_shift > 0)
                {
                    r += m_shift;
                    r %= m_base;
                }
                m_tmp.append(m_symbols.at(r));
            }
        }

        if (m_whitespace)
        {
            r = skMax<int>(0, m_charsPerBase - (int)m_tmp.size());
            for (q = 0; q < r; ++q)
                print(' ');
            print(' ');
        }
        skString::ReverseIterator it = m_tmp.reverseIterator();
        while (it.hasMoreElements())
            print(it.getNext());
    }

    void print(SKuint8 inp)
    {
        putchar(inp);
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
