#ifndef PRIM_H
#define PRIM_H

#include <cstdint>
#include <cstdlib>

using u64 = std::uint64_t;
using u32 = std::uint32_t;
using u16 = std::uint16_t;
using u8 = std::uint8_t;

enum class TokKind : u8 {
    String,
    Symbol,
    Punct,
    Endl,
    Eof,
};

struct TokArray {
    u32 size{}, cap;
    TokKind *kinds;
    u32 *starts;
    u32 *ends;
    TokArray(u32 n) : cap(n) {
        kinds = static_cast<TokKind *>(malloc(n * sizeof(TokKind)));
        starts = static_cast<u32 *>(malloc(n * sizeof(u32)));
        ends = static_cast<u32 *>(malloc(n * sizeof(u32)));
    }
    ~TokArray() {
        std::free(kinds);
        std::free(starts);
        std::free(ends);
    }
    void push(TokKind tk, u32 s, u32 e) {
        kinds[size] = tk;
        starts[size] = s;
        ends[size] = e;
        size++;
    }
};

#endif
