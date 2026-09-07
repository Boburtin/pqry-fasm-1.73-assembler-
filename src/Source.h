#ifndef SOURCE_H
#define SOURCE_H

#include <fstream>
#include <iterator>
#include <string>

#include "STLAlias.inc"

class Source {
  public:
    Source(std::string &&str) : bytes(str) {
        if (bytes.size() >= 3 && static_cast<uchar>(bytes[0]) == 0xEF &&
            static_cast<uchar>(bytes[1]) == 0xBB && static_cast<uchar>(bytes[2]) == 0xBF)
            bytes.erase(0, 3);
    }
    static Source fromFile(const char *fp) {
        std::ifstream file(fp, std::ios::binary);
        auto res = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return Source(std::move(res));
    }

    u32 size() const { return bytes.size(); }
    char at(u32 idx) const { return bytes[idx]; }
    const char *data() const { return bytes.data(); }

  private:
    std::string bytes;
};

#endif
