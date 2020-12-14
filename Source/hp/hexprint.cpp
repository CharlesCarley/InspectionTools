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
#include "Utils/skLogger.h"
#include "Utils/skMap.h"
#include "Utils/skMemoryStream.h"
#include "Utils/skString.h"

namespace skCommandLine
{
    // clang-format off
#define lowercase \
         'a' : case 'b' : case 'c' : case 'd' : case 'e' : case 'f' : \
    case 'g' : case 'h' : case 'i' : case 'j' : case 'k' : case 'l' : \
    case 'm' : case 'n' : case 'o' : case 'p' : case 'q' : case 'r' : \
    case 's' : case 't' : case 'u' : case 'v' : case 'w' : case 'x' : \
    case 'y' : case 'z'

#define uppercase \
         'A' : case 'B' : case 'C' : case 'D' : case 'E' : case 'F' : \
    case 'G' : case 'H' : case 'I' : case 'J' : case 'K' : case 'L' : \
    case 'M' : case 'N' : case 'O' : case 'P' : case 'Q' : case 'R' : \
    case 'S' : case 'T' : case 'U' : case 'V' : case 'W' : case 'X' : \
    case 'Y' : case 'Z'

#define digit \
         '0' : case '1' : case '2' : case '3' : \
    case '4' : case '5' : case '6' : case '7' : \
    case '8' : case '9'

    // clang-format on

    enum TokenType
    {
        TOK_NONE,
        TOK_SWITCH_SHORT,
        TOK_SWITCH_LONG,
        TOK_NEXT,
        TOK_OPTION,
        TOK_INTEGER,
        TOK_REAL,
        TOK_IDENTIFIER,
        TOK_FILEPATH,
        TOK_EOS,
        TOK_ERROR
    };


    class Token
    {
    private:
        int      m_type;
        skString m_value;

    public:
        Token() :
            m_type(TOK_NONE)
        {
            m_value.reserve(64);
        }

        inline const int &getType()
        {
            return m_type;
        }

        inline void setType(const int &type)
        {
            m_type = type;
        }

        inline const skString &getValue()
        {
            return m_value;
        }


        void append(char ch)
        {
            m_value.append(ch);
        }
        void append(const char *str)
        {
            m_value.append(str);
        }

        void append(const skString &str)
        {
            m_value.append(str);
        }

        void clear()
        {
            m_type = TOK_EOS;
            m_value.resize(0);
        }
    };


    class Scanner
    {
    private:
        skString m_value;
        SKsize   m_pos;


    public:
        Scanner() :
            m_pos(0)
        {
            m_value.reserve(64);
        }

        void clear()
        {
            m_value.resize(0);
        }
        void append(const char *arg)
        {
            m_value.append(arg);
            m_value.append(' ');
        }

        void lex(Token &tok)
        {
            tok.clear();

            while (m_pos < m_value.size())
            {
                char ch = m_value.at(m_pos++);
                switch (ch)
                {
                case '-':
                {
                    char nx = m_value.at(m_pos);
                    if (nx == '-')
                        ++m_pos;
                    tok.setType(nx == '-' ? (int)TOK_SWITCH_SHORT : (int)TOK_SWITCH_LONG);
                    return;
                }
                case digit:
                case uppercase:
                case lowercase:
                    while (ch != 0 && ch != ' ')
                    {
                        tok.append(ch);
                        ch = m_value.at(m_pos++);
                    }
                    if (ch == ' ')
                        --m_pos;
                    tok.setType(TOK_IDENTIFIER);
                    return;
                case ' ':
                case '\t':
                    tok.setType(TOK_NEXT);
                    return;
                default:
                    tok.setType(TOK_ERROR);
                    tok.append(m_value.substr(m_pos-1, m_value.size()));
                    return;
                }
            }
        }

        inline const skString &getValue()
        {
            return m_value;
        }
    };

    class Option
    {
    private:
        skString m_name;
        skString m_value;
        skString m_description;

    public:
        Option()
        {
        }
        ~Option()
        {
        }

        inline const skString &getName()
        {
            return m_name;
        }

        inline void setName(const skString &str)
        {
            m_name.assign(str);
        }

        inline const skString &getValue()
        {
            return m_value;
        }

        inline void setValue(const skString &str)
        {
            m_value.assign(str);
        }

        inline const skString &getDescription()
        {
            return m_description;
        }

        inline void setDescription(const skString &str)
        {
            m_description.assign(str);
        }
    };


    class Parser
    {
    public:
        typedef skHashTable<skString, Option *> Options;
        typedef skArray<skString>               List;

    private:
        skString m_program;
        int      m_base;
        Scanner  m_scanner;
        Options  m_switches;

        int getBaseName(const char *input)
        {
            int offs = 0;
            if (input)
            {
                int len = (int)skStringUtils::length(input), i;
                for (i = len - 1; i >= 0 && offs == 0; --i)
                    if (input[i] == '/' || input[i] == '\\')
                        offs = i + 1;
            }
            return offs;
        }

    public:
        Parser() :
            m_base(0)
        {
        }

        int parse(int argc, char **argv)
        {
            m_program.clear();
            m_program.append(argv[0]);
            m_base = getBaseName(argv[0]);

            m_scanner.clear();

            for (int i = 1; i < argc; ++i)
                m_scanner.append(argv[i]);


            Token a, b, c, d;

            while (a.getType() != TOK_EOS)
            {
                m_scanner.lex(a);
                int type = a.getType();

                if (a.getType()==TOK_ERROR)
                {
                    skLogf(LD_ERROR, "\nError %s\n\n", a.getValue().c_str());
                    usage();
                    return -1;
                }



                if (type == TOK_SWITCH_SHORT || type == TOK_SWITCH_LONG)
                {
                    // grab the next token
                    m_scanner.lex(a);

                    if (a.getType() == TOK_NEXT || a.getType() == TOK_EOS)
                    {
                        skLogf(LD_ERROR, "Expected a switch argument\n");
                        usage();
                        return -1;
                    }
                    else
                    {
                        // > next && < TOK_EOS
                        SKsize pos = m_switches.find(a.getValue());
                        if (pos == SK_NPOS)
                        {
                            skLogf(LD_ERROR, "Unknown option '%s'\n", a.getValue().c_str());
                            usage();
                            return -1;
                        }
                    }
                }
            }
            return 0;
        }

        void logInput()
        {
            skLogf(LD_INFO, "\n%s %s\n\n", getBaseProgram().c_str(), m_scanner.getValue().c_str());
        }

        const skString &getProgram()
        {
            return m_program;
        }

        skString getBaseProgram()
        {
            skString rval;
            m_program.substr(rval, m_base, m_program.size());
            return rval;
        }


        void addOption(const skString &name, const skString &desc)
        {
            SKsize pos = m_switches.find(name);
            if (pos != SK_NPOS)
                skLogf(LD_WARN, "Duplicate option '%s'\n", name.c_str());
            else
            {
                Option *opt = new Option();
                opt->setName(name);
                opt->setDescription(desc);
                m_switches.insert(name, opt);
            }
        }


        void usage()
        {
            skLogf(LD_INFO, "Usage: %s <options>\n\n", getBaseProgram().c_str());
            skLogf(LD_INFO, "    Where options is any of the following.\n\n");


            Options::Iterator it = m_switches.iterator();
            while (it.hasMoreElements())
            {
                Option *opt = it.getNext().second;
                skLogf(LD_INFO, "        -%s    %s\n", opt->getName().c_str(), opt->getDescription().c_str());
            }
        }

    };


}  // namespace skCommandLine

int main(int argc, char **argv)
{
    skLogger log;
    log.setFlags(LF_STDOUT);
    log.setDetail(LD_VERBOSE);


    skCommandLine::Parser psr;
    psr.addOption("a", "a description about the option.");
    psr.addOption("b", "a description about the option.");
    psr.addOption("C", "a description about the option.");
    if (psr.parse(argc, argv) < 0)
        return 1;
    
    psr.logInput();
    return 0;
}
