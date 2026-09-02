#ifndef TOK_H
#define TOK_H

#include "Prim.h"
#include "Source.h"
#include "TokUtils.h"

class TokMaker {
  public:
    TokMaker(const Source &src) : src_(src) {}
    void scan(TokArray &out) {
        for (;;) {
            auto [k, s, e] = next();
            out.push(k, s, e);
            if (k == TokKind::Eof)
                break;
        }
    }

  private:
    static bool isPunct(char c) {
        return c == '+' || c == '-' || c == '/' || c == '*' || c == '=' ||
               c == '<' || c == '>' || c == '(' || c == ')' || c == '[' ||
               c == ']' || c == '{' || c == '}' || c == ':' || c == ',' ||
               c == '|' || c == '&' || c == '~' || c == '#' || c == '`';
    }
    char get() { return src_.at(p_); }
    struct Tok {
        TokKind kind;
        u32 start;
        u32 end;
    };
    Tok next() {
        while (isSpace(src_.at(p_)))
            p_++;

        char c = get();
        auto start = p_;

        switch (c) {
        case '\0':
        case 0x1A:
            return {TokKind::Eof, p_, p_};
        case '\'':
        case '"': {
            p_++;
            char d = get();
            while (!isEOF(d) && d != c) {
                p_++;
                d = get();
            }
            if (d == c)
                // advance past the closing quote
                p_++;
            return {TokKind::String, start, p_};
        }
        case '\r':
            p_ += src_.at(p_ + 1) == '\n' ? 2 : 1;
            return {TokKind::Endl, start, p_};
        case '\n':
            p_ += src_.at(p_ + 1) == '\r' ? 2 : 1;
            return {TokKind::Endl, start, p_};
        default:
            if (isPunct(c)) {
                p_++;
                // single char punct tokens' end will point to next token start
                return {TokKind::Punct, start, p_};
            }
            while (!isEOF(c) && !isSpace(c) && !isEOL(c)) {
                p_++;
                c = get();
            }
            // don't advance p_ here as it likely points to eof/space/eol
            return {TokKind::Symbol, start, p_};
        }
    }
    const Source &src_;
    u32 p_{};
};

#endif
