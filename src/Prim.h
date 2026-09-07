#ifndef PRIM_H
#define PRIM_H

#include "IntegralAliases.h"

#include <string>

constexpr u32 U32_MAX = 0xFFFFFFFFU;
constexpr u32 kMaxExpand = 1U << 12;
constexpr uSize kCacheLine = 64ULL;

#define CUR(x) x[size]

#if defined(_WIN32)
#include <malloc.h>
#define ALIGNED_ALLOC(align, size) _aligned_malloc((size), (align))
#define ALIGNED_REALLOC(p, newSz, align) _aligned_realloc((p), (newSz), (align))
#define ALIGNED_FREE(p) _aligned_free(p)
#else
#include <cstdlib>
#include <cstring>
#define ALIGNED_ALLOC(align, size) std::aligned_alloc((align), (size))
#define ALIGNED_FREE(p) std::free(p)
#endif

#if defined(__cpp_lib_start_lifetime_as)
#include <memory>
#define HAVE_START_LIFETIME_AS 1
#else
#define HAVE_START_LIFETIME_AS 0
#endif

#if defined(__cpp_lib_flat_map)
#include <flat_map>
template <class K, class V> using T_Map = std::flat_map<K, V>;
#else
#include <unordered_map>
template <class K, class V> using T_Map = std::unordered_map<K, V>;
#endif

#if __has_cpp_attribute(assume)
#define ASSUME(expr) [[assume(expr)]]
#else
#define ASSUME(expr) ((void)0)
#endif

template <class T> inline T *alloc_array(void *p, u32 n)
{
#if HAVE_START_LIFETIME_AS
    return std::start_lifetime_as_array<T>(p, n);
#else
    return static_cast<T *>(p);
#endif
}

inline void *aligned_grow(void *old_p, uSize oldBytes, uSize newBytes, uSize align)
{
#if defined(_WIN32)
    return ALIGNED_REALLOC(old_p, newBytes, align);
#else
    void *p = ALIGNED_ALLOC(align, newBytes);
    if (old_p)
    {
        std::memcpy(p, old_p, oldBytes < newBytes ? oldBytes : newBytes);
        ALIGNED_FREE(old_p);
    }
    return p;
#endif
}

inline uSize aligned_size(uSize n)
{
    return (n + kCacheLine - 1) & ~(kCacheLine - 1);
}

enum class TokKind : u8 { String, Symbol, Punct, Endl, Eof };
enum class DefKind : u8 { Equ, Define, Fix };
enum class MacroKind : u8 { Macro, Struc };
enum class ParamMode : u8 { Plain, Greedy, Group };

struct TokArray
{
    u32 size{}, cap{};
    TokKind *kind{};
    u32 *start{}, *end{};

    TokArray() = default;
    explicit TokArray(u32 n)
    {
        reserve(n);
    }
    TokArray(const TokArray &) = delete;
    TokArray &operator=(const TokArray &) = delete;

    ~TokArray()
    {
        ALIGNED_FREE(kind);
        ALIGNED_FREE(start);
        ALIGNED_FREE(end);
    }

    void reserve(u32 n)
    {
        kind = alloc_array<TokKind>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(TokKind))), n);
        start = alloc_array<u32>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(u32))), n);
        end = alloc_array<u32>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(u32))), n);
        cap = n;
    }

    void grow()
    {
        u32 n = cap ? cap * 2 : 64;
        kind = alloc_array<TokKind>(aligned_grow(kind, cap * sizeof(TokKind), aligned_size(n * sizeof(TokKind)), kCacheLine), n);
        start = alloc_array<u32>(aligned_grow(start, cap * sizeof(u32), aligned_size(n * sizeof(u32)), kCacheLine), n);
        end = alloc_array<u32>(aligned_grow(end, cap * sizeof(u32), aligned_size(n * sizeof(u32)), kCacheLine), n);
        cap = n;
    }

    u32 push(TokKind k, u32 s, u32 e)
    {
        ASSUME(size <= cap);
        if (size == cap) grow();
        CUR(kind) = k;
        CUR(start) = s;
        CUR(end) = e;
        return size++;
    }
};

struct DefTable
{
    u32 size{}, cap{};
    DefKind *kind{};
    u32 *valBeg{};
    u32 *valEnd{};
    u32 *lock{};
    u32 *prev{};
    TokArray val;
    T_Map<std::string, u32> byName;

    DefTable() = default;
    explicit DefTable(u32 n)
    {
        reserve(n);
    }
    DefTable(const DefTable &) = delete;
    DefTable &operator=(const DefTable &) = delete;
    ~DefTable()
    {
        ALIGNED_FREE(kind);
        ALIGNED_FREE(valBeg);
        ALIGNED_FREE(valEnd);
        ALIGNED_FREE(lock);
        ALIGNED_FREE(prev);
    }
    void reserve(u32 n)
    {
        kind = alloc_array<DefKind>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(DefKind))), n);
        valBeg = alloc_array<u32>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(u32))), n);
        valEnd = alloc_array<u32>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(u32))), n);
        lock = alloc_array<u32>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(u32))), n);
        prev = alloc_array<u32>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(u32))), n);
        cap = n;
    }
    void grow()
    {
        u32 n = cap ? cap * 2 : 32;
        kind = alloc_array<DefKind>(aligned_grow(kind, cap * sizeof(DefKind), aligned_size(n * sizeof(DefKind)), kCacheLine), n);
        valBeg = alloc_array<u32>(aligned_grow(valBeg, cap * sizeof(u32), aligned_size(n * sizeof(u32)), kCacheLine), n);
        valEnd = alloc_array<u32>(aligned_grow(valEnd, cap * sizeof(u32), aligned_size(n * sizeof(u32)), kCacheLine), n);
        lock = alloc_array<u32>(aligned_grow(lock, cap * sizeof(u32), aligned_size(n * sizeof(u32)), kCacheLine), n);
        prev = alloc_array<u32>(aligned_grow(prev, cap * sizeof(u32), aligned_size(n * sizeof(u32)), kCacheLine), n);
        cap = n;
    }
    u32 add(DefKind k, u32 vb, u32 ve, u32 pr = U32_MAX)
    {
        ASSUME(size <= cap);
        if (size == cap) grow();
        CUR(kind) = k;
        CUR(valBeg) = vb;
        CUR(valEnd) = ve;
        CUR(lock) = 0;
        CUR(prev) = pr;
        return size++;
    }
};

struct ParamStore
{
    u32 size{}, cap{};
    u32 *nameOff{};
    u32 *nameLen{};
    ParamMode *mode{};
    u32 *defltBeg{}; // [defltBeg,defltEnd) window into MacroTable::body; U32_MAX = no default
    u32 *defltEnd{};

    ParamStore() = default;
    explicit ParamStore(u32 n)
    {
        reserve(n);
    }
    ParamStore(const ParamStore &) = delete;
    ParamStore &operator=(const ParamStore &) = delete;
    ~ParamStore()
    {
        ALIGNED_FREE(nameOff);
        ALIGNED_FREE(nameLen);
        ALIGNED_FREE(mode);
        ALIGNED_FREE(defltBeg);
        ALIGNED_FREE(defltEnd);
    }
    void reserve(u32 n)
    {
        nameOff = alloc_array<u32>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(u32))), n);
        nameLen = alloc_array<u32>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(u32))), n);
        mode = alloc_array<ParamMode>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(ParamMode))), n);
        defltBeg = alloc_array<u32>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(u32))), n);
        defltEnd = alloc_array<u32>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(u32))), n);
        cap = n;
    }

    void grow()
    {
        u32 n = cap ? cap * 2 : 32;
        nameOff = alloc_array<u32>(aligned_grow(nameOff, cap * sizeof(u32), aligned_size(n * sizeof(u32)), kCacheLine), n);
        nameLen = alloc_array<u32>(aligned_grow(nameLen, cap * sizeof(u32), aligned_size(n * sizeof(u32)), kCacheLine), n);
        mode = alloc_array<ParamMode>(aligned_grow(mode, cap * sizeof(ParamMode), aligned_size(n * sizeof(ParamMode)), kCacheLine), n);
        defltBeg = alloc_array<u32>(aligned_grow(defltBeg, cap * sizeof(u32), aligned_size(n * sizeof(u32)), kCacheLine), n);
        defltEnd = alloc_array<u32>(aligned_grow(defltEnd, cap * sizeof(u32), aligned_size(n * sizeof(u32)), kCacheLine), n);
        cap = n;
    }
    u32 push(u32 no, u32 nl, ParamMode m, u32 db = U32_MAX, u32 de = U32_MAX)
    {
        ASSUME(size <= cap);
        if (size == cap) grow();
        CUR(nameOff) = no;
        CUR(nameLen) = nl;
        CUR(mode) = m;
        CUR(defltBeg) = db;
        CUR(defltEnd) = de;
        return size++;
    }
};

struct MacroTable
{
    u32 size{}, cap{};
    MacroKind *kind{};
    u32 *paramBeg{};
    u32 *paramEnd{};
    u32 *bodyBeg{};
    u32 *bodyEnd{};

    ParamStore params;
    TokArray body;
    T_Map<std::string, u32> byName;

    MacroTable() = default;
    explicit MacroTable(u32 n)
    {
        reserve(n);
    }
    MacroTable(const MacroTable &) = delete;
    MacroTable &operator=(const MacroTable &) = delete;
    ~MacroTable()
    {
        ALIGNED_FREE(kind);
        ALIGNED_FREE(paramBeg);
        ALIGNED_FREE(paramEnd);
        ALIGNED_FREE(bodyBeg);
        ALIGNED_FREE(bodyEnd);
    }
    void reserve(u32 n)
    {
        kind = alloc_array<MacroKind>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(MacroKind))), n);
        paramBeg = alloc_array<u32>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(u32))), n);
        paramEnd = alloc_array<u32>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(u32))), n);
        bodyBeg = alloc_array<u32>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(u32))), n);
        bodyEnd = alloc_array<u32>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(u32))), n);
        cap = n;
    }
    void grow()
    {
        u32 n = cap ? cap * 2 : 16;
        kind = alloc_array<MacroKind>(aligned_grow(kind, cap * sizeof(MacroKind), aligned_size(n * sizeof(MacroKind)), kCacheLine), n);
        paramBeg = alloc_array<u32>(aligned_grow(paramBeg, cap * sizeof(u32), aligned_size(n * sizeof(u32)), kCacheLine), n);
        paramEnd = alloc_array<u32>(aligned_grow(paramEnd, cap * sizeof(u32), aligned_size(n * sizeof(u32)), kCacheLine), n);
        bodyBeg = alloc_array<u32>(aligned_grow(bodyBeg, cap * sizeof(u32), aligned_size(n * sizeof(u32)), kCacheLine), n);
        bodyEnd = alloc_array<u32>(aligned_grow(bodyEnd, cap * sizeof(u32), aligned_size(n * sizeof(u32)), kCacheLine), n);
        cap = n;
    }
    u32 add(MacroKind k, u32 pb, u32 pe, u32 bb, u32 be)
    {
        ASSUME(size <= cap);
        if (size == cap) grow();
        CUR(kind) = k;
        CUR(paramBeg) = pb;
        CUR(paramEnd) = pe;
        CUR(bodyBeg) = bb;
        CUR(bodyEnd) = be;
        return size++;
    }
};

struct OriginArray
{
    u32 size{}, cap{};
    u32 *srcTok{};
    u32 *depth{};

    OriginArray() = default;
    OriginArray(const OriginArray &) = delete;
    OriginArray &operator=(const OriginArray &) = delete;
    ~OriginArray()
    {
        ALIGNED_FREE(srcTok);
        ALIGNED_FREE(depth);
    }

    void reserve(u32 n)
    {
        srcTok = alloc_array<u32>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(u32))), n);
        depth = alloc_array<u32>(ALIGNED_ALLOC(kCacheLine, aligned_size(n * sizeof(u32))), n);
        cap = n;
    }

    void grow()
    {
        u32 n = cap ? cap * 2 : 256;
        srcTok = alloc_array<u32>(aligned_grow(srcTok, cap * sizeof(u32), aligned_size(n * sizeof(u32)), kCacheLine), n);
        depth = alloc_array<u32>(aligned_grow(depth, cap * sizeof(u32), aligned_size(n * sizeof(u32)), kCacheLine), n);
        cap = n;
    }

    void push(u32 s, u32 d)
    {
        ASSUME(size <= cap);
        if (size == cap) grow();
        CUR(srcTok) = s;
        CUR(depth) = d;
        size++;
    }
};

struct TextArena
{
    std::string bytes;
    u32 put(const char *p, u32 n)
    {
        u32 off = bytes.size();
        bytes.append(p, n);
        return off;
    }
    const char *data() const
    {
        return bytes.data();
    }
};

#endif
