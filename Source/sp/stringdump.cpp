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
    SP_SHOW_ADDRESS=0,
    SP_LOWER,
    SP_UPPER,
    SP_DIGITS,
    SP_HEX,
    SP_BASE64,
    SP_MERGE,
    SP_LENGTH,
    SP_RANGE,
    SP_MAX
};

const Switch Switches[SP_MAX] = {
    {
        SP_SHOW_ADDRESS,
        0,
        "show-address",
        "Display the start address of each string.",
        true,
        0,
    },
    {
        SP_LOWER,
        'l',
        "lowercase",
        "Print the [a-z] character set.",
        true,
        0,
    },
    {
        SP_UPPER,
        'u',
        "uppercase",
        "Print the [A-Z] character set.",
        true,
        0,
    },
    {
        SP_DIGITS,
        'd',
        "digit",
        "Print the [0-9] character set.",
        true,
        0,
    },
    {
        SP_HEX,
        0,
        "hex",
        "Print the [0-9Aa-Ff] character set.\n"
        "  - Takes precedence over other filters.",
        true,
        0,
    },
    {
        SP_BASE64,
        0,
        "base64",
        "Print the [0-9Aa-Zz+/=] character set.\n"
        "  - Takes precedence over other filters.",
        true,
        0,
    },
    {
        SP_MERGE,
        'm',
        "merge",
        "Merge the string printout",
        true,
        1,
    },
    {
        SP_LENGTH,
        'n',
        "number",
        "Set a minimum string length",
        true,
        1,
    },
    {
        SP_RANGE,
        'r',
        "range",
        "Specify a start address and a range.\n"
        "Arguments: [address, range]\n"
        "      - address Base 16 [0 - file length]\n"
        "      - range   Base 10 [0 - file length]\n",
        true,
        2,
    },
};

class Application
{
private:
    skFileStream m_stream;
    SKuint32     m_addressRange[2];
    SKuint32     m_number;
    bool         m_upperCase;
    bool         m_lowercaseCase;
    bool         m_digit;
    bool         m_hex;
    bool         m_base64;
    bool         m_logAddress;
    SKuint32     m_merge;

public:
    Application() :
        m_addressRange(),
        m_number(SK_NPOS32),
        m_upperCase(false),
        m_lowercaseCase(false),
        m_digit(false),
        m_hex(false),
        m_base64(),
        m_logAddress(false),
        m_merge(SK_NPOS32)
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
        if (psr.parse(argc, argv, Switches, SP_MAX) < 0)
            return 1;

        m_logAddress    = psr.isPresent(SP_SHOW_ADDRESS);
        m_digit         = psr.isPresent(SP_DIGITS);
        m_lowercaseCase = psr.isPresent(SP_LOWER);
        m_upperCase     = psr.isPresent(SP_UPPER);
        m_hex           = psr.isPresent(SP_HEX);
        m_base64        = psr.isPresent(SP_BASE64);

        if (!m_logAddress && psr.isPresent(SP_MERGE))
            m_merge = psr.getValueInt(SP_MERGE, 0, 0);

        if (psr.isPresent(SP_LENGTH))
            m_number = psr.getValueInt(SP_LENGTH, 0, 0);

        if (psr.isPresent(SP_RANGE))
        {
            m_addressRange[0] = psr.getValueInt(SP_RANGE, 0, SK_NPOS32, 16);
            m_addressRange[1] = psr.getValueInt(SP_RANGE, 1, SK_NPOS32, 10);
        }

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

    bool filterChar(char ch)
    {
        bool result = false;
        if (m_base64)
        {
            result = ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch >= '0' && ch <= '9';
            result = result || ch == '+' || ch == '/' || ch == '=';
            return result;
        }
        else if (m_hex)
        {
            result = ch >= 'a' && ch <= 'f' || ch >= 'A' && ch <= 'F' || ch >= '0' && ch <= '9';
            return result;
        }
        else if (!m_lowercaseCase && !m_upperCase && !m_digit)
            return ch >= 32 && ch < 127;

        if (m_lowercaseCase)
            result = ch >= 'a' && ch <= 'z';
        if (m_upperCase)
            result = result || ch >= 'A' && ch <= 'Z';
        if (m_digit)
            result = result || ch >= '0' && ch <= '9';
        return result;
    }

    int print()
    {
        SKuint8 buffer[1025];

        skString tmpStr;
        tmpStr.reserve(1024);

        SKsize n;
        SKsize a, r;
        n = m_stream.size();
        a = skClamp<SKsize>(m_addressRange[0], 0, n);
        r = skClamp<SKsize>(m_addressRange[1], 0, n);

        if (m_addressRange[0] != SK_NPOS32)
            m_stream.seek(a, SEEK_SET);
        else
            r = n;

        SKuint64 address = 0;

        SKsize br, tr = 0, i, m = 0;
        while (!m_stream.eof() && tr < r)
        {
            br = m_stream.read(buffer, skMin<SKsize>(1024, r));
            if (br != SK_NPOS32 && br > 0)
            {
                buffer[br] = 0;
                for (i = 0; i < br; ++i)
                {
                    const char ch = (char)buffer[i];
                    if (filterChar(ch))
                    {
                        if (tmpStr.empty())
                            address = tr + i;

                        tmpStr.append(ch);
                        if (m_merge != SK_NPOS32 && m_merge > 0)
                        {
                            if (m++ % m_merge == 0)
                                tmpStr.append('\n');
                        }
                    }
                    else if (!tmpStr.empty())
                    {
                        if (m_number != SK_NPOS32)
                        {
                            if (tmpStr.size() >= m_number)
                            {
                                if (m_logAddress)
                                    printf("%08X  ", (SKuint32)address);

                                printf("%s", tmpStr.c_str());
                                if (m_merge == SK_NPOS32)
                                    putchar('\n');
                            }
                        }
                        else
                        {
                            if (m_logAddress)
                                printf("%08X  ", (SKuint32)address);

                            printf("%s", tmpStr.c_str());

                            if (m_merge == SK_NPOS32)
                                putchar('\n');
                        }

                        tmpStr.resize(0);
                    }
                }
            }

            tr += br;
        }
        putchar('\n');
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
