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
#include "Utils/skString.h"

using namespace skHexPrint;
using namespace skCommandLine;

struct ProgramInfo
{
    skFileStream m_stream;
    SKint64      m_code;
    SKuint32     m_addressRange[2];
    SKuint32     m_flags;
};

const skCommandLine::Switch Switches[] = {
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
};



void HexPrint(ProgramInfo &ctx)
{
    skFileStream &fp = ctx.m_stream;
    SKuint8       buffer[1025];

    SKsize n;
    SKsize a, r;
    n = fp.size();
    a = skClamp<SKsize>(ctx.m_addressRange[0], 0, n);
    r = skClamp<SKsize>(ctx.m_addressRange[1], 0, n);

    if (ctx.m_addressRange[0] != -1)
        fp.seek(a, SEEK_SET);
    else
    {
        a = 0;
        r = n;
    }

    SKsize br,
        tr = 0;
    while (!fp.eof() && tr < r)
    {
        br = fp.read(buffer, skMin<SKsize>(1024, r));
        if (br != -1 && br > 0)
        {
            buffer[br] = 0;
            dumpHex(buffer, tr + a, br, ctx.m_flags, ctx.m_code);
            tr += br;
        }
    }
}


int main(int argc, char **argv)
{
    skLogger log;
    log.setFlags(LF_STDOUT);
    log.setDetail(LD_VERBOSE);

    Parser psr;
    psr.initializeSwitches(Switches, sizeof(Switches) / sizeof(Switches[0]));

    if (psr.parse(argc, argv) < 0)
        return 1;

    ProgramInfo info = {skFileStream(), -1, {(SKuint32)-1, (SKuint32)-1}, PF_DEFAULT | PF_FULLADDR};

    if (psr.isPresent("flags"))
    {
        ParseOption *op = psr.getOption("flags");
        info.m_flags    = op->getInt();
    }

    if (psr.isPresent("mark"))
    {
        ParseOption *op = psr.getOption("mark");
        info.m_code = op->getInt64(0, 16);
    }
    if (psr.isPresent("no-color"))
        info.m_flags &= ~PF_COLORIZE;

    if (psr.isPresent("range"))
    {
        ParseOption *op = psr.getOption("range");
        info.m_addressRange[0] = op->getInt(0, 16);
        info.m_addressRange[1] = op->getInt(1, 10);
    }


    using List = skCommandLine::Parser::List;
    List &args = psr.getArgList();
    if (args.empty())
    {
        skLogf(LD_ERROR, "No files supplied\n");
        return 1;
    }

    info.m_stream.open(args[0].c_str(), skStream::READ);
    if (!info.m_stream.isOpen())
    {
        skLogf(LD_ERROR, "Failed to open file %s\n", args[0].c_str());
        return 1;
    }

    HexPrint(info);
    return 0;
}
