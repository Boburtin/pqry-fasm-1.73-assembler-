#ifndef PREPROC_H
#define PREPROC_H

#include <unordered_map>

#include "Prim.h"
#include "Source.h"

struct Preproc {
    const Source& src;
    const TokArray& in;
    TextArena arena;
    TokArray out;
    OriginArray origin;

    DefTable defs;
    MacroTable macros;

    u64 localCtr = 0;
    u32 depth = 0;
    u32 i = 0;

    std::string_view view(u32 i) const {
        return {src.data() + in.starts[i], in.ends[i] - in.starts[i]};
    }

    void emitRaw(u32 j) {
        auto t = view(j);
        u32 off = arena.put(t.data(), (u32)t.size());
        out.push(in.kinds[j], off, off + (u32)t.size());
        origin.push(j, depth);
    }

    u32 lineEnd(u32 b) const {
        u32 j = b;
        for (;;) {
            TokKind k = in.kinds[j];
            if (k == TokKind::Eof) return j;
            if (k == TokKind::Endl) {
                bool cont = j > b && in.kinds[j - 1] == TokKind::Symbol && view(j - 1) == "\\";
                if (!cont) return j;
            }
            j++;
        }
    }

    void run() {
        while (i < in.size) {
            TokKind k = in.kinds[i];
            if (k == TokKind::Eof) {
                emitRaw(i);
                break;
            }
            if (k == TokKind::Endl) {
                emitRaw(i);
                ++i;
                continue;
            }
            u32 b = i, e = lineEnd(i);
            for (u32 j = b; j < e; ++j) {
                if (in.kinds[j] == TokKind::Symbol && view(j) == "\\" && j + 1 < e &&
                    in.kinds[j + 1] == TokKind::Endl) {
                    j++;
                    continue;
                }
                emitRaw(j);
            }
            i = e;
        }
    }
};

#endif