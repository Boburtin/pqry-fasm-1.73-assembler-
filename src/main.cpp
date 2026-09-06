#include <cstdio>

#include "IntegralAliases.h"
#include "Preproc.h"
#include "Prim.h"
#include "Source.h"
#include "TokUtils.h"

#define EXIT_USAGE 2

const char *BOLD = "\033[1m[%05llu]\033[0m %s";

const char *chSuccess = "[+]";
const char *chFailure = "[-]";
const char *chWarning = "[!]";
const char *chUpdate = "[*]";

const char *Eof = "\033[1;32mEOF!\033[0m";
const char *Punct = "\033[3;31mPunct\033[0m";
const char *Endl = "\033[1;33mEndl\033[0m";
const char *String = "\033[3;30mString\033[0m";
const char *Symbol = "\033[3;34mSymbol\033[0m";

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "\n\033[1;36m%s usage:\033[35m pqry <file>\033[0m\n", chFailure);
        return EXIT_USAGE;
    }

    auto src = Source::fromFile(argv[1]);
    auto toks = TokArray(src.size() + 1);

    TMaker(src).scan(toks);
    Preproc preproc{src, toks};
    preproc.run();

    auto tknPrint = [](const TokArray &t) {
        for (uSize i{}; i < t.size; ++i)
        {
            switch (t.kinds[i])
            {
            case TokKind::Symbol:
                printf(BOLD, i, Symbol);
                break;
            case TokKind::Endl:
                putchar('\n');
                break;
            case TokKind::Eof:
                printf(BOLD, i, Eof);
                break;
            case TokKind::Punct:
                printf(BOLD, i, Punct);
                break;
            case TokKind::String:
                printf(BOLD, i, String);
                break;
            }
        }
    };

    tknPrint(toks);
    puts("\n\n");
    tknPrint(preproc.out);

    return 0;
}
