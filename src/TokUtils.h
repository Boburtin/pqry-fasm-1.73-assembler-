#ifndef TOK_UTILS_H
#define TOK_UTILS_H

inline bool isEOF(char c) { return c == '\0' || c == 0x1A; }
inline bool isSpace(char c) { return c == ' ' || c == '\t'; }
inline bool isEOL(char c) { return c == '\r' || c == '\n'; }
inline bool isSymbol(char c) {
    return (c >= 0x30 && c <= 0x39) || (c >= 65 && c <= 90) ||
           (c >= 97 && c <= 122) || c == '_' || c == '.' || c == '$' ||
           c == '!' || c == '%' || c == '@' || c == '?';
}

#endif
