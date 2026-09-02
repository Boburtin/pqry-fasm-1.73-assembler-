#include "Source.h"
#include "Prim.h"
#include "Tok.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        return 2;
    }
    auto src = Source::fromFile(argv[1]);
    auto toks = TokArray(src.size() + 1);
    auto tmaker = TokMaker(src);
    tmaker.scan(toks);
    return 0;
}
