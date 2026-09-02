#ifndef TOK_UTILS_H
#define TOK_UTILS_H

#include "Prim.h"
#include "Source.h"

class TMaker {
   public:
    static bool isEOF(char c) { return c == '\0' || c == 0x1A; }
    static bool isSpace(char c) { return c == ' ' || c == '\t'; }
    static bool isEOL(char c) { return c == '\r' || c == '\n'; }
    static bool isSymbol(char c) {
        return (c >= 0x30 && c <= 0x39) || (c >= 65 && c <= 90) || (c >= 97 && c <= 122) ||
               c == '_' || c == '.' || c == '$' || c == '!' || c == '%' || c == '@' || c == '?';
    }
    static bool isPunct(char c) {
        return c == '+' || c == '-' || c == '/' || c == '*' || c == '=' || c == '<' || c == '>' ||
               c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == ':' ||
               c == ',' || c == '|' || c == '&' || c == '~' || c == '#' || c == '`';
    }
    struct Tkn {
        TokKind kind;
        u32 start, end;
    };

    TMaker(const Source& src) : src_(src) {}

    char get() const { return src_.at(idx_); }

    void scan(TokArray& out) {
        for (;;) {
            auto [k, s, e] = next();
            out.push(k, s, e);
            if (k == TokKind::Eof) break;
        }
    }

   private:
    Tkn next() {
        /* skip tabs & spaces */
        while (isSpace(get())) idx_++;

        /* get non-space char and pin start */
        char c{get()};
        u32 start{idx_};

        switch (c) {
            case 0x1A:  // do not advance idx_
            case 0x00:
                return {TokKind::Eof, idx_, idx_};
            case 0x27:  // quotes
            case 0x22: {
                idx_++;  // point to first char
                char d{get()};
                while (!isEOF(d) && c != d) {
                    idx_++;
                    d = get();  // adv and fetch until d = <', ", EOF>
                }
                return {TokKind::String, start, (d == c ? ++idx_ : idx_)};
            }
            case 0x0A:  // LF/CR
            case 0x0D: {
                idx_ += (src_.at(idx_ + 1) ^ c == 0x07 ? 2 : 1);
                return {TokKind::Endl, start, idx_};
            }
            default:
                if (isPunct(c)) return {TokKind::Punct, start, ++idx_};
                while (!isEOF(c) && !isSpace(c) && !isEOL(c)) {
                    idx_++;
                    c = get();
                }
                return {TokKind::Symbol, start, idx_};
        }
    }
    const Source& src_;
    u32 idx_ {};
};

#endif