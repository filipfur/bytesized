#pragma once

#include <cstddef>
#include <string>

namespace dex {
inline static constexpr size_t strlen(const char *path) {
    for (size_t i{0}; i < __UINT64_MAX__; ++i) {
        if (path[i] == '\0') {
            return i;
        }
    }
    return __UINT64_MAX__;
}

inline static constexpr const char *filename(const char *path) {
    auto len = strlen(path);
    if (len < 0) {
        return nullptr;
    }
    for (size_t i{len - 1}; i > 0; --i) {
        if (i == 0) {
            return nullptr;
        }
        if (path[i] == '/' || path[i] == '\\') {
            return path + i + 1;
        }
    }
    return nullptr;
}

inline static constexpr void replace(std::string &str, char a, char b) {
    for (size_t i{0}; i < str.length(); ++i) {
        if (str[i] == a) {
            str[i] = b;
        }
    }
}

inline static constexpr char to_hex(char c) { return c > 9 ? ('a' + c - 10) : (c + '0'); }

} // namespace dex