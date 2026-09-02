#include <cstdio>

#include "Prim.h"
#include "Source.h"
#include "Tok.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        return 2;
    }
    auto src = Source::fromFile(argv[1]);
    auto toks = TokArray(src.size() + 1);
    auto tmaker = TokMaker(src);
    tmaker.scan(toks);

    for (u64 i{}; i < toks.size; ++i) {
        switch (toks.kinds[i]) {
            case TokKind::Symbol:
                printf("[%llu] = Symbol\n", i);
                break;
            case TokKind::Endl:
                printf("[%llu] = Endl\n", i);
                break;
            case TokKind::Eof:
                printf("[%llu] = Eof\n", i);
                break;
            case TokKind::Punct:
                printf("[%llu] = Punct\n", i);
                break;
            case TokKind::String:
                printf("[%llu] = String\n", i);
                break;
        }
    }
    return 0;
}
