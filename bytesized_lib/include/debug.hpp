#pragma once

#include <glm/glm.hpp>

#define DO_ONCE(block)                                                                             \
    do {                                                                                           \
        static bool _first{true};                                                                  \
        if (_first) {                                                                              \
            block;                                                                                 \
            _first = false;                                                                        \
        }                                                                                          \
    } while (0)

static inline void print(const glm::mat4 &m4) {
    printf("|%.1f %.1f %.1f %.1f|\n", m4[0][0], m4[0][1], m4[0][2], m4[0][3]);
    printf("|%.1f %.1f %.1f %.1f|\n", m4[1][0], m4[1][1], m4[1][2], m4[1][3]);
    printf("|%.1f %.1f %.1f %.1f|\n", m4[2][0], m4[2][1], m4[2][2], m4[2][3]);
    printf("|%.1f %.1f %.1f %.1f|\n", m4[3][0], m4[3][1], m4[3][2], m4[3][3]);
}