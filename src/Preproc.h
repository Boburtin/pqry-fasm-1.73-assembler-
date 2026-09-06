#ifndef PREPROC_H
#define PREPROC_H

#include <unordered_map>
#include <utility>
#include <vector>

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

    u64 localCtr{};
    u32 depth{};
    u32 i{};

    Preproc(const Source& s, const TokArray& t) : src(s), in(t) {
        out.reserve(t.size);
        origin.reserve(t.size);
        arena.bytes.reserve(s.size());
    }

    static bool ieq(std::string_view a, std::string_view b) {
        if (a.size() != b.size()) return false;
        for (auto j = b.size(); j-- > 0;)
            if ((a[j] | 0x20) != b[j]) return false;
        return true;
    }

    static std::string fold(std::string_view s) {
        std::string r(s);
        for (char& c : r) c |= 0x20;
        return r;
    }

    std::string_view view(u32 j) const {
        return {src.data() + in.starts[j], in.ends[j] - in.starts[j]};
    }

    void emitRaw(u32 j) {
        auto t = view(j);
        u32 off = arena.put(t.data(), (u32)t.size());
        out.push(in.kinds[j], off, off + (u32)t.size());
        origin.push(j, depth);
    }

    void emitText(TokKind k, std::string_view t) {
        u32 off = arena.put(t.data(), (u32)t.size());
        out.push(k, off, off + (u32)t.size());
        origin.push(U32_MAX, depth);
    }

    void emitStored(TokKind k, u32 off, u32 len) {
        out.push(k, off, off + len);
        origin.push(U32_MAX, depth);
    }

    void emitExpanded(u32 j) {
        if (in.kinds[j] != TokKind::Symbol) {
            emitRaw(j);
            return;
        }

        auto it = defs.byName.find(fold(view(j)));
        if (it == defs.byName.end() || defs.lock[it->second]) {
            emitRaw(j);
            return;
        }

        u32 id = it->second;
        defs.lock[id]++;
        for (u32 n = defs.valBeg[id]; n < defs.valEnd[id]; ++n)
            emitStored(defs.val.kinds[n], defs.val.starts[n],
                       defs.val.ends[n] - defs.val.starts[n]);
        defs.lock[id]--;
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

    void defineEqu(const std::string& name, u32 b, u32 e) {
        u32 vb = defs.val.size;
        for (u32 j = b; j < e; ++j) {
            auto it = defs.byName.find(fold(view(j)));
            if (it != defs.byName.end() && defs.lock[it->second] == 0) {
                u32 s = it->second;
                for (u32 n = defs.valBeg[s]; n < defs.valEnd[s]; ++n)
                    defs.val.push(defs.val.kinds[n], defs.val.starts[n], defs.val.ends[n]);
            } else {
                auto t = view(j);
                u32 off = arena.put(t.data(), (u32)t.size());
                defs.val.push(in.kinds[j], off, off + (u32)t.size());
            }
        }
        u32 ve = defs.val.size;

        u32 prevId = U32_MAX;
        if (auto it = defs.byName.find(name); it != defs.byName.end()) prevId = it->second;

        defs.byName[name] = defs.add(DefKind::Equ, vb, ve, prevId);

        i = e;
    }

    void doRestore(u32 b, u32 e) {
        for (u32 j = b; j < e; ++j) {
            if (in.kinds[j] == TokKind::Punct && view(j) == ",") continue;
            std::string name = fold(view(j));
            auto it = defs.byName.find(name);
            if (it == defs.byName.end()) continue;
            u32 id = it->second;
            if (defs.prev[id] != U32_MAX)
                defs.byName[name] = defs.prev[id];
            else
                defs.byName.erase(it);
        }
        i = e;
    }

    void captureMacro(u32 b) {
        std::string name = fold(view(b + 1));
        u32 j = b + 2;
        u32 pb = macros.params.size;

        for (;; ++j) {
            auto ckind = in.kinds[j];
            auto cview = view(j);
            if (j >= in.size || ckind == TokKind::Eof) {
                i = j;
                return;
            }
            if (ckind == TokKind::Endl) continue;
            if (ckind == TokKind::Punct) {
                if (cview == "{") break;
                if (cview == ",") continue;
            }
            ParamMode cmode = ParamMode::Plain;
            if (!cview.empty() && cview.back() == '*') {
                cmode = ParamMode::Greedy;
                cview.remove_suffix(1);
            }
            std::string folded = fold(cview);
            u32 nOff{arena.put(folded.data(), (u32)folded.size())};
            u32 dBeg{U32_MAX}, dEnd{U32_MAX};
            if (j + 1 < in.size && in.kinds[j + 1] == TokKind::Punct && view(j + 1) == ":") {
                j += 2;
                dBeg = macros.body.size;
                ckind = in.kinds[j];
                cview = view(j);
                while (j < in.size && ckind != TokKind::Eof && ckind != TokKind::Endl &&
                       !(ckind == TokKind::Punct && (cview == "," || cview == "{"))) {
                    cview = view(j);
                    ckind = in.kinds[j];
                    u32 off{arena.put(cview.data(), (u32)cview.size())};
                    macros.body.push(ckind, off, off + (u32)cview.size());
                    ++j;
                }
                dEnd = macros.body.size;
                --j;
            }
            macros.params.push(nOff, (u32)folded.size(), cmode, dBeg, dEnd);
        }
        u32 pe = macros.params.size;
        u32 bb = macros.body.size;
        int braceDepth = 1;
        ++j;
        for (;; ++j) {
            auto ckind = in.kinds[j];
            auto cview = view(j);
            if (j >= in.size || ckind == TokKind::Eof) break;
            if (ckind == TokKind::Punct) {
                if (cview == "{")
                    ++braceDepth;
                else if (cview == "}" && --braceDepth == 0) {
                    ++j;
                    break;
                }
            }
            u32 off = arena.put(cview.data(), (u32)cview.size());
            macros.body.push(ckind, off, off + (u32)cview.size());
        }
        u32 be = macros.body.size;
        macros.byName[name] = macros.add(MacroKind::Macro, pb, pe, bb, be);
        i = j;
    }

    void expandMacro(u32 id, u32 b, u32 e) {
        if (depth + 1 > kMaxExpand) {  // TODO: recursion error handling
            i = e;
            return;
        }
        u32 pb{macros.paramBeg[id]}, pe{macros.paramEnd[id]};
        u32 bb{macros.bodyBeg[id]}, be{macros.bodyEnd[id]};
        u32 nParams{pe - pb};

        std::vector<std::pair<u32, u32>> arg(nParams, {U32_MAX, U32_MAX});
        u32 j{b + 1}, p{};
        while (j < e && p < nParams) {
            bool greedy{macros.params.mode[pb + p] == ParamMode::Greedy};
            u32 ab{j};
            if (greedy) {
                j = e;
            } else {
                while (j < e && !(in.kinds[j] == TokKind::Punct && view(j) == ",")) ++j;
            }
            arg[p] = {ab, j};
            if (j < e && in.kinds[j] == TokKind::Punct && view(j) == ",") ++j;
            ++p;
        }

        ++depth;
        for (u32 n{bb}; n < be; ++n) {
            if (macros.body.kinds[n] == TokKind::Symbol) {
                std::string_view t(arena.data() + macros.body.starts[n],
                                   macros.body.ends[n] - macros.body.starts[n]);
                std::string folded = fold(t);
                u32 matched{U32_MAX};
                for (u32 q{}; q < nParams; ++q) {
                    std::string_view pn(arena.data() + macros.params.nameOff[pb + q],
                                        macros.params.nameLen[pb + q]);
                    if (folded == pn) {
                        matched = q;
                        break;
                    }
                }
                if (matched != U32_MAX) {
                    if (arg[matched].first != U32_MAX) {
                        for (u32 k{arg[matched].first}; k < arg[matched].second; ++k)
                            emitExpanded(k);
                    } else {
                        u32 dBeg{macros.params.defltBeg[pb + matched]},
                            dEnd{macros.params.defltEnd[pb + matched]};
                        for (u32 n{dBeg}; n != U32_MAX && n < dEnd; ++n) {
                            emitStored(macros.body.kinds[n], macros.body.starts[n],
                                       macros.body.ends[n] - macros.body.starts[n]);
                        }
                    }
                    continue;
                }
            }
            emitStored(macros.body.kinds[n], macros.body.starts[n],
                       macros.body.ends[n] - macros.body.starts[n]);
        }
        --depth;
        i = e;
    }

    bool dispatchDirective(u32 b, u32 e) {
        if (in.kinds[b] == TokKind::Symbol && ieq(view(b), "macro")) {
            captureMacro(b);
            return true;
        }
        if (in.kinds[b] == TokKind::Symbol && ieq(view(b), "restore")) {
            doRestore(b + 1, e);
            return true;
        }
        if (b + 1 < e && in.kinds[b] == TokKind::Symbol &&
            ((in.kinds[b + 1] == TokKind::Symbol && ieq(view(b + 1), "equ")) ||
             (in.kinds[b + 1] == TokKind::Punct && view(b + 1) == "="))) {
            defineEqu(fold(view(b)), b + 2, e);
            return true;
        }
        return false;
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
            if (dispatchDirective(b, e)) continue;
            for (u32 j = b; j < e; ++j) {
                if (in.kinds[j] == TokKind::Symbol && view(j) == "\\" && j + 1 < e &&
                    in.kinds[j + 1] == TokKind::Endl) {
                    j++;
                    continue;
                }
                emitExpanded(j);
            }
            i = e;
        }
    }
};

#endif