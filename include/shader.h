#pragma once

#include "embed.h"
#include "uniform.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace gpu {

struct Shader {
    uint32_t id;
};

struct ShaderProgram {
    uint32_t id;
    Shader *vertex;
    Shader *fragment;
    std::unordered_map<std::string, Uniform> uniforms;
    Uniform *uniform(const char *key);

    void use();
};

bool Shader_compile(const gpu::Shader &id, const char *src);
void Shader_createProgram(ShaderProgram &shaderProgram);

#define __BUILTINSHADERS                                                                           \
    __SHADER(SCREEN_VERT, __shaders__vert_screen, GL_VERTEX_SHADER)                                \
    __SHADER(OBJECT_VERT, __shaders__vert_object, GL_VERTEX_SHADER)                                \
    __SHADER(ANIM_VERT, __shaders__vert_anim, GL_VERTEX_SHADER)                                    \
    __SHADER(UI_VERT, __shaders__vert_ui, GL_VERTEX_SHADER)                                        \
    __SHADER(TEXT_VERT, __shaders__vert_text, GL_VERTEX_SHADER)                                    \
    __SHADER(TEXTURE_FRAG, __shaders__frag_texture, GL_FRAGMENT_SHADER)                            \
    __SHADER(OBJECT_FRAG, __shaders__frag_object, GL_FRAGMENT_SHADER)                              \
    __SHADER(ANIM_FRAG, __shaders__frag_anim, GL_FRAGMENT_SHADER)                                  \
    __SHADER(TEXT_FRAG, __shaders__frag_text, GL_FRAGMENT_SHADER)

enum BuiltinShader {
#define __SHADER(label, str, type) label,
    __BUILTINSHADERS
#undef __SHADER
        SHADER_COUNT,
};

gpu::Shader *builtinShader(BuiltinShader builtinShader);

} // namespace gpu