#include "bdf.h"

#include "recycler.hpp"
#include <cstdlib>
#include <string_view>

static recycler<bdf::Font, 5> FONTS = {};

static bool is_int(char c) { return (c >= '0' && c <= '9') || c == '-'; }

static int parse_int(const char *data, size_t &i) {
    assert(is_int(data[i]));
    size_t start = i;
    while (is_int(data[i])) {
        ++i;
    }
    return std::atoi(data + start);
}

static uint8_t char_to_hex(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    assert(c >= 'A' && c <= 'F');
    return c - 'A' + 10;
}

bdf::Font *bdf::createFont(const char *data, size_t length) {
    bdf::Font *font = FONTS.acquire();
    size_t row_start = 0;
    std::string_view str;
    uint8_t glyph{0};
    uint8_t *bitmapPtr;
    bool ignore{false};
    for (size_t i{0}; i < length; ++i) {
        if (ignore) {
            ;
        } else if (str.length() == 0 && (data[i] == ' ' || data[i] == '\n')) {
            str = {data + row_start, i - row_start};
            if (str == "SIZE") {
                ++i;
                font->pointsize = parse_int(data, i);
                ++i;
                font->dpix = parse_int(data, i);
                ++i;
                font->dpiy = parse_int(data, i);
            } else if (str == "FONTBOUNDINGBOX") {
                ++i;
                font->pw = parse_int(data, i);
                ++i;
                font->ph = parse_int(data, i);
                ++i;
                font->px = parse_int(data, i);
                ++i;
                font->py = parse_int(data, i);
            } else if (str == "BITMAP") {
                ++i;
                bitmapPtr = font->bitmap[glyph++];
                for (size_t j{0}; j < font->pointsize; ++j) {
                    *bitmapPtr = (char_to_hex(data[i]) << 4 | char_to_hex(data[i + 1]));
                    i += 3;
                    ++bitmapPtr;
                }
                if (glyph > 127) {
                    break; // exit
                }
            } else {
                ignore = true;
            }
        }
        if (data[i] == '\n') {
            row_start = i + 1;
            ignore = false;
            str = {};
        }
    }
    return font;
}

void bdf::freeFont(bdf::Font *font) { FONTS.free(font); }