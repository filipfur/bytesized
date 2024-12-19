#pragma once

#include "gcem.hpp"
#include "gpu.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <glm/glm.hpp>

namespace gpu {
// clang-format off
static const float screen_vertices[] = {
    -1.0f, -1.0f, 0.0f, 0.0f,
    +1.0f, -1.0f, 1.0f, 0.0f,
    -1.0f, +1.0f, 0.0f, 1.0f,
    +1.0f, +1.0f, 1.0f, 1.0f,
};
static const uint16_t screen_indices[] = {
    0, 1, 2,
    1, 3, 2
};
// clang-format on

/*constexpr std::pair<std::array<float, 10>, std::array<uint16_t, 10>> Cube() {
    std::array<float, 10> vs;
    std::array<uint16_t, 10> is;
}*/

/*

| | | |
| | | |
| | | |

*/

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

template <std::size_t N, std::size_t M> struct Plane {
    static constexpr std::size_t CELLS_N = N + 1;
    static constexpr std::size_t CELLS_M = M + 1;
    static constexpr std::size_t CELLS = CELLS_N * CELLS_M;
    constexpr Plane(float uvScale = 1.0f) : vertices{}, indices{} {
        for (size_t m{0}; m < CELLS_M; ++m) {
            for (size_t n{0}; n < CELLS_N; ++n) {
                int i = m * CELLS_N + n;
                float s = (float)n / N;
                float t = (float)m / M;
                vertices[i].position = {
                    s * 2.0f - 1.0f,
                    t * 2.0f - 1.0f,
                    0.0f,
                };
                vertices[i].normal = {0.0f, 0.0f, 1.0f};
                vertices[i].uv = {s * uvScale, t * uvScale};
            }
        }
        size_t i = 0;
        size_t j = 0;
        for (size_t m{0}; m < M; ++m) {
            for (size_t n{0}; n < N; ++n) {
                indices[i * 6 + 0] = j + 0;
                indices[i * 6 + 1] = j + 1;
                indices[i * 6 + 2] = j + CELLS_N;
                indices[i * 6 + 3] = j + 1;
                indices[i * 6 + 4] = j + CELLS_N + 1;
                indices[i * 6 + 5] = j + CELLS_N;
                ++i;
                ++j;
            }
            ++j;
        }
    }
    Vertex vertices[CELLS];
    uint16_t indices[CELLS * 6];
};

struct Cube {

    constexpr Cube() : vertices{}, indices{0, 1, 2, 0, 3, 2} {
        /*uint32_t i{0};
        for (size_t z{0}; z < 2; ++z) {
            for (size_t y{0}; y < 2; ++y) {
                for (size_t x{0}; x < 2; ++x) {
                    vertices[i].position = {x, y, z};
                    vertices[i].normal = glm::normalize(glm::vec3{x, y, z});
                    vertices[i].uv = {x * 0.5f + 0.5f, y * 0.5f + 0.5f};
                }
            }
        }*/
        size_t i{0};
        size_t j{0};
        for (glm::vec3 normal : {
                 glm::vec3{1.0f, 0.0f, 0.0f},
                 glm::vec3{0.0f, 1.0f, 0.0f},
                 glm::vec3{0.0f, 0.0f, 1.0f},
                 glm::vec3{-1.0f, 0.0f, 0.0f},
                 glm::vec3{0.0f, -1.0f, 0.0f},
                 glm::vec3{0.0f, 0.0f, -1.0f},
             }) {
            glm::vec3 right =
                glm::cross(normal.y * normal.y < FLT_EPSILON ? glm::vec3{0.0f, 1.0f, 0.0f}
                                                             : glm::vec3{1.0f, 0.0f, 0.0f},
                           normal);
            glm::vec3 up = glm::cross(normal, right);
            vertices[i + 0].position = normal - right - up;
            vertices[i + 0].normal = normal;
            vertices[i + 0].uv = {0.0f, 0.0f};

            vertices[i + 1].position = normal + right - up;
            vertices[i + 1].normal = normal;
            vertices[i + 1].uv = {1.0f, 0.0f};

            vertices[i + 2].position = normal - right + up;
            vertices[i + 2].normal = normal;
            vertices[i + 2].uv = {0.0f, 1.0f};

            vertices[i + 3].position = normal + right + up;
            vertices[i + 3].normal = normal;
            vertices[i + 3].uv = {1.0f, 1.0f};

            indices[j + 0] = i + 0;
            indices[j + 1] = i + 1;
            indices[j + 2] = i + 2;
            indices[j + 3] = i + 1;
            indices[j + 4] = i + 3;
            indices[j + 5] = i + 2;

            i += 4;
            j += 6;
        }
    }

    Vertex vertices[6 * 4];
    uint16_t indices[6 * 6];
};

template <std::size_t segments, std::size_t discs> struct Sphere {
    static constexpr std::size_t CELLS_N = segments + 1;
    static constexpr std::size_t CELLS_M = discs + 1;
    static constexpr std::size_t CELLS = CELLS_N * CELLS_M;

    enum Options {
        NONE = 0x00,
        CAP_A = (1 << 0),
        CAP_B = (1 << 1),
        CONE_A = (1 << 2),
        CONE_B = (1 << 3),
        CYLINDER = (1 << 4),
    };
    constexpr Sphere(Options options = NONE) : vertices{}, indices{} {
        float s = 1.0f / static_cast<float>(segments);
        uint16_t vs_i = 0;
        float theta = M_PI / static_cast<float>(discs);

        for (uint16_t j{0}; j <= discs; ++j) {
            float y, r;
            if (options & CYLINDER) {
                r = 1.0f;
                y = -1.0f + (float)j / discs * 2.0f;
            } else if (options & CONE_A) {
                r = (float)j / discs;
                y = -1.0f + (float)j / discs * 2.0f;
            } else if (options & CONE_B) {
                r = 1.0f - (float)j / discs;
                y = -1.0f + (float)j / discs * 2.0f;
            } else {
                y = -gcem::cos(theta * j);
                float f = 1.0 - y * y;
                r = f < FLT_EPSILON ? 0.0f : gcem::sqrt(f) + FLT_EPSILON;
            }
            for (uint16_t i{0}; i <= segments; ++i) {
                float a = 2.0f * M_PI * i * s;
                glm::vec3 v{gcem::sin(a) * r, y, gcem::cos(a) * r};
                vertices[vs_i].position = v;
                vertices[vs_i].normal = v;
                vertices[vs_i].uv = {i * s, y * 0.5f + 0.5f};
                ++vs_i;
            }
        }

        size_t i{0};
        size_t j{0};
        for (uint16_t d{0}; d < discs; ++d) {
            for (uint16_t s{0}; s < segments; ++s) {
                indices[i * 6 + 0] = j + 0;
                indices[i * 6 + 1] = j + 1;
                indices[i * 6 + 2] = j + CELLS_N;
                indices[i * 6 + 3] = j + 1;
                indices[i * 6 + 4] = j + CELLS_N + 1;
                indices[i * 6 + 5] = j + CELLS_N;
                ++i;
                ++j;
            }
            ++j;
        }
    }

    Vertex vertices[CELLS];
    uint16_t indices[segments * discs * 6];
};

gpu::Primitive *getBuiltinPrimitive(gpu::BuiltinPrimitive builtinPrimitive);

void createBuiltinPrimitives();

} // namespace gpu