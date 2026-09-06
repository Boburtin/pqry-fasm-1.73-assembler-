#ifndef PRIM_H
#define PRIM_H

#include <cstdint>
#include <cstdlib>
#include <string>
#include <unordered_map>

using u64 = std::uint64_t;
using u32 = std::uint32_t;
using u16 = std::uint16_t;
using u8 = std::uint8_t;

constexpr u32 U32_MAX = 0xFFFFFFFFU;
constexpr u32 kMaxExpand = 1u << 12;

enum class TokKind : u8 {
    String,
    Symbol,
    Punct,
    Endl,
    Eof,
};
enum class DefKind : u8 { Equ, Define, Fix };
enum class MacroKind : u8 { Macro, Struc };
enum class ParamMode : u8 { Plain, Greedy, Group };

struct TokArray {
    u32 size{}, cap{};
    TokKind* kinds{};
    u32* starts{};
    u32* ends{};

    TokArray() = default;
    explicit TokArray(u32 n) { reserve(n); }
    TokArray(const TokArray&) = delete;
    TokArray& operator=(const TokArray&) = delete;

    ~TokArray() {
        std::free(kinds);
        std::free(starts);
        std::free(ends);
    }

    void reserve(u32 n) {
        kinds = static_cast<TokKind*>(malloc(n * sizeof(TokKind)));
        starts = static_cast<u32*>(malloc(n * sizeof(u32)));
        ends = static_cast<u32*>(malloc(n * sizeof(u32)));
        cap = n;
    }

    void grow() {
        u32 n = cap ? cap * 2 : 64;
        kinds = static_cast<TokKind*>(std::realloc(kinds, n * sizeof(TokKind)));
        starts = static_cast<u32*>(std::realloc(starts, n * sizeof(u32)));
        ends = static_cast<u32*>(std::realloc(ends, n * sizeof(u32)));
        cap = n;
    }

    u32 push(TokKind k, u32 s, u32 e) {
        if (size == cap) grow();
        kinds[size] = k;
        starts[size] = s;
        ends[size] = e;
        return size++;
    }
};

struct DefTable {
    u32 size{}, cap{};
    DefKind* kind{};
    u32* valBeg{};
    u32* valEnd{};
    u32* lock{};
    u32* prev{};
    TokArray val;
    std::unordered_map<std::string, u32> byName;
    /* TODO! implement better hashmap */

    DefTable() = default;
    explicit DefTable(u32 n) { reserve(n); }
    DefTable(const DefTable&) = delete;
    DefTable& operator=(const DefTable&) = delete;
    ~DefTable() {
        std::free(kind);
        std::free(valBeg);
        std::free(valEnd);
        std::free(lock);
        std::free(prev);
    }
    void reserve(u32 n) {
        kind = static_cast<DefKind*>(std::malloc(n * sizeof(DefKind)));
        valBeg = static_cast<u32*>(std::malloc(n * sizeof(u32)));
        valEnd = static_cast<u32*>(std::malloc(n * sizeof(u32)));
        lock = static_cast<u32*>(std::malloc(n * sizeof(u32)));
        prev = static_cast<u32*>(std::malloc(n * sizeof(u32)));
        cap = n;
    }
    void grow() {
        u32 n = cap ? cap * 2 : 32;
        kind = static_cast<DefKind*>(std::realloc(kind, n * sizeof(DefKind)));
        valBeg = static_cast<u32*>(std::realloc(valBeg, n * sizeof(u32)));
        valEnd = static_cast<u32*>(std::realloc(valEnd, n * sizeof(u32)));
        lock = static_cast<u32*>(std::realloc(lock, n * sizeof(u32)));
        prev = static_cast<u32*>(std::realloc(prev, n * sizeof(u32)));
        cap = n;
    }
    u32 add(DefKind k, u32 vb, u32 ve, u32 prevId = U32_MAX) {
        if (size == cap) grow();
        kind[size] = k;
        valBeg[size] = vb;
        valEnd[size] = ve;
        lock[size] = 0;
        prev[size] = prevId;
        return size++;
    }
};

struct ParamStore {
    u32 size{}, cap{};
    u32* nameOff{};
    u32* nameLen{};
    ParamMode* mode{};
    u32* defltBeg{};   // [defltBeg,defltEnd) window into MacroTable::body; U32_MAX = no default
    u32* defltEnd{};

    ParamStore() = default;
    explicit ParamStore(u32 n) { reserve(n); }
    ParamStore(const ParamStore&) = delete;
    ParamStore& operator=(const ParamStore&) = delete;
    ~ParamStore() {
        std::free(nameOff);
        std::free(nameLen);
        std::free(mode);
        std::free(defltBeg);
        std::free(defltEnd);
    }
    void reserve(u32 n) {
        nameOff = static_cast<u32*>(std::malloc(n * sizeof(u32)));
        nameLen = static_cast<u32*>(std::malloc(n * sizeof(u32)));
        mode = static_cast<ParamMode*>(std::malloc(n * sizeof(ParamMode)));
        defltBeg = static_cast<u32*>(std::malloc(n * sizeof(u32)));
        defltEnd = static_cast<u32*>(std::malloc(n * sizeof(u32)));
        cap = n;
    }

    void grow() {
        u32 n = cap ? cap * 2 : 32;
        nameOff = static_cast<u32*>(std::realloc(nameOff, n * sizeof(u32)));
        nameLen = static_cast<u32*>(std::realloc(nameLen, n * sizeof(u32)));
        mode = static_cast<ParamMode*>(std::realloc(mode, n * sizeof(ParamMode)));
        defltBeg = static_cast<u32*>(std::realloc(defltBeg, n * sizeof(u32)));
        defltEnd = static_cast<u32*>(std::realloc(defltEnd, n * sizeof(u32)));
        cap = n;
    }
    u32 push(u32 nOff, u32 nLen, ParamMode m, u32 dBeg = U32_MAX, u32 dEnd = U32_MAX) {
        if (size == cap) grow();
        nameOff[size] = nOff;
        nameLen[size] = nLen;
        mode[size] = m;
        defltBeg[size] = dBeg;
        defltEnd[size] = dEnd;
        return size++;
    }
};

struct MacroTable {
    u32 size{}, cap{};
    MacroKind* kind{};
    u32* paramBeg{};
    u32* paramEnd{};
    u32* bodyBeg{};
    u32* bodyEnd{};

    ParamStore params;
    TokArray body;
    std::unordered_map<std::string, u32> byName;

    MacroTable() = default;
    explicit MacroTable(u32 n) { reserve(n); }
    MacroTable(const MacroTable&) = delete;
    MacroTable& operator=(const MacroTable&) = delete;
    ~MacroTable() {
        std::free(kind);
        std::free(paramBeg);
        std::free(paramEnd);
        std::free(bodyBeg);
        std::free(bodyEnd);
    }
    void reserve(u32 n) {
        kind = static_cast<MacroKind*>(std::malloc(n * sizeof(MacroKind)));
        paramBeg = static_cast<u32*>(std::malloc(n * sizeof(u32)));
        paramEnd = static_cast<u32*>(std::malloc(n * sizeof(u32)));
        bodyBeg = static_cast<u32*>(std::malloc(n * sizeof(u32)));
        bodyEnd = static_cast<u32*>(std::malloc(n * sizeof(u32)));
        cap = n;
    }
    void grow() {
        u32 n = cap ? cap * 2 : 16;
        kind = static_cast<MacroKind*>(std::realloc(kind, n * sizeof(MacroKind)));
        paramBeg = static_cast<u32*>(std::realloc(paramBeg, n * sizeof(u32)));
        paramEnd = static_cast<u32*>(std::realloc(paramEnd, n * sizeof(u32)));
        bodyBeg = static_cast<u32*>(std::realloc(bodyBeg, n * sizeof(u32)));
        bodyEnd = static_cast<u32*>(std::realloc(bodyEnd, n * sizeof(u32)));
        cap = n;
    }
    u32 add(MacroKind k, u32 pb, u32 pe, u32 bb, u32 be) {
        if (size == cap) grow();
        kind[size] = k;
        paramBeg[size] = pb;
        paramEnd[size] = pe;
        bodyBeg[size] = bb;
        bodyEnd[size] = be;
        return size++;
    }
};

struct OriginArray {
    u32 size{}, cap{};
    u32* srcTok{};
    u32* depth{};

    OriginArray() = default;
    OriginArray(const OriginArray&) = delete;
    OriginArray& operator=(const OriginArray&) = delete;
    ~OriginArray() {
        std::free(srcTok);
        std::free(depth);
    }

    void reserve(u32 n) {
        srcTok = static_cast<u32*>(std::malloc(n * sizeof(u32)));
        depth = static_cast<u32*>(std::malloc(n * sizeof(u32)));
        cap = n;
    }

    void grow() {
        u32 n = cap ? cap * 2 : 256;
        srcTok = static_cast<u32*>(std::realloc(srcTok, n * sizeof(u32)));
        depth = static_cast<u32*>(std::realloc(depth, n * sizeof(u32)));
        cap = n;
    }

    void push(u32 s, u32 d) {
        if (size == cap) grow();
        srcTok[size] = s;
        depth[size] = d;
        size++;
    }
};

struct TextArena {
    std::string bytes;
    u32 put(const char* p, u32 n) {
        u32 off = bytes.size();
        bytes.append(p, n);
        return off;
    }
    const char *data() const { return bytes.data(); }
};

#endif
