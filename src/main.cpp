#include <cstdio>

#include "Preproc.h"
#include "Prim.h"
#include "Source.h"
#include "TokUtils.h"

const char* Eof = "\033[1;32mEOF!\033[0m";
const char* Punct = "\033[3;31mPunct\033[0m";
const char* Endl = "\033[1;33mEndl\033[0m";
const char* String = "\033[3;30mString\033[0m";
const char* Symbol = "\033[3;34mSymbol\033[0m";

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "\n\033[1;36m[!] usage:\033[35m pqry <file>\033[0m\n");
        return 2;
    }

    auto src = Source::fromFile(argv[1]);
    auto toks = TokArray(src.size() + 1);
    auto tmaker = TMaker(src);
    tmaker.scan(toks);

    for (u64 i{}; i < toks.size; ++i) {
        switch (toks.kinds[i]) {
            case TokKind::Symbol:
                printf("\n[%llu] = %s", i, Symbol);
                break;
            case TokKind::Endl:
                printf(", [%llu] %s", i, Endl);
                break;
            case TokKind::Eof:
                printf("\n[%llu] = %s", i, Eof);
                break;
            case TokKind::Punct:
                printf("\n[%llu] = %s", i, Punct);
                break;
            case TokKind::String:
                printf("\n[%llu] = %s", i, String);
                break;
        }
    }
    return 0;
}
