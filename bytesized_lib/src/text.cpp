#include "gpu.h"

#include <cstring>

void gpu::Text::init() {

    // clang-format off
    static float vertices[] = {
        -1.0f, -1.0f, +0.0f,     0.0f, 0.0f, 1.0f,     0.0f, 0.0f,
        +1.0f, -1.0f, +0.0f,     0.0f, 0.0f, 1.0f,     1.0f, 0.0f,
        -1.0f, +1.0f, +0.0f,     0.0f, 0.0f, 1.0f,     0.0f, 1.0f,
        +1.0f, +1.0f, +0.0f,     0.0f, 0.0f, 1.0f,     1.0f, 1.0f,
    };
    static uint16_t indices[] = {
        0, 1, 2,
        1, 3, 2
    };
    // clang-format on

    auto &[primitive, material] = node->mesh->primitives.front();
    uint32_t *vbo = primitive->vbos.front();
    primitive->vao->bind();
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *primitive->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

    glBindVertexArray(0);
    primitive->count = sizeof(indices) / sizeof(uint16_t);
}

void gpu::Text::setText(const char *value_, bool center) {
    value = value_;

    const size_t V = 4;
    const size_t A = 8;
    static std::array<float, V * A * 1024> vertices = {};
    static std::array<uint16_t, 6 * 1024> indices = {};
    size_t letters{0};
    if (center) {
        letters = strlen(value);
    }
    float u = 1.0f / 16.0f;
    float v = 1.0f / 8.0f;
    float w = bdfFont->pw;
    float h = bdfFont->ph;
    float dx = bdfFont->px;
    float dy = bdfFont->py;

    float x = 0.0f;
    float y = 0.0f;
    if (center) {
        x -= letters * w * 0.5f;
        y -= h * 0.5f;
    }
    float s;
    float t;

    for (size_t i{0}; i < 1024; ++i) {
        char cc = value[i];
        if (cc == '\0') {
            letters = i;
            break;
        }
        /*if (cc == ' ') {
            x += bdfFont->pw;
            continue;
        }*/
        size_t ii = i * (V * A);
        s = (cc % 16) * u;
        t = (7 - cc / 16) * v;

        // 2-3
        // 0-1

        // vert0
        vertices[ii + 0] = dx + x;
        vertices[ii + 1] = dy + y;
        vertices[ii + 6] = s;
        vertices[ii + 7] = t;

        // vert 1
        vertices[ii + 8] = dx + x + w;
        vertices[ii + 9] = dy + y;
        vertices[ii + 14] = s + u;
        vertices[ii + 15] = t;

        // vert 2
        vertices[ii + 16] = dx + x;
        vertices[ii + 17] = dy + y + h;
        vertices[ii + 22] = s;
        vertices[ii + 23] = t + v;

        // vert 3
        vertices[ii + 24] = dx + x + w;
        vertices[ii + 25] = dy + y + h;
        vertices[ii + 30] = s + u;
        vertices[ii + 31] = t + v;

        size_t ij = i * 6;
        uint16_t quadIndex = i * 4;
        indices[ij + 0] = quadIndex + 0;
        indices[ij + 1] = quadIndex + 1;
        indices[ij + 2] = quadIndex + 2;
        indices[ij + 3] = quadIndex + 1;
        indices[ij + 4] = quadIndex + 3;
        indices[ij + 5] = quadIndex + 2;
        x += bdfFont->pw;
    }

    auto &[primitive, material] = node->mesh->primitives.front();
    primitive->vao->bind();
    uint32_t *vbo = primitive->vbos.front();
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, letters * V * A * sizeof(float), vertices.data(),
                 GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *primitive->ebo);
    uint32_t elementCount = letters * 6;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementCount * sizeof(uint16_t), indices.data(),
                 GL_DYNAMIC_DRAW);

    glBindVertexArray(0);
    primitive->count = elementCount;
}

float gpu::Text::width() const { return bdfFont->pw * strlen(value); }
float gpu::Text::height() const { return bdfFont->ph; }