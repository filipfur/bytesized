#pragma once

#include "embed/anim_frag.hpp"
#include "embed/anim_vert.hpp"
#include "embed/billboard_vert.hpp"
#include "embed/object_frag.hpp"
#include "embed/object_vert.hpp"
#include "embed/screen_vert.hpp"
#include "embed/text_frag.hpp"
#include "embed/text_vert.hpp"
#include "embed/texture_frag.hpp"
#include "embed/ui_vert.hpp"
#include "gpu.h"
#include "uniform.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace builtin {

#define __BUILTINSHADERS                                                                           \
    __SHADER(SCREEN_VERT, _embed_screen_vert, GL_VERTEX_SHADER)                                    \
    __SHADER(OBJECT_VERT, _embed_object_vert, GL_VERTEX_SHADER)                                    \
    __SHADER(ANIM_VERT, _embed_anim_vert, GL_VERTEX_SHADER)                                        \
    __SHADER(TEXT_VERT, _embed_text_vert, GL_VERTEX_SHADER)                                        \
    __SHADER(TEXTURE_FRAG, _embed_texture_frag, GL_FRAGMENT_SHADER)                                \
    __SHADER(OBJECT_FRAG, _embed_object_frag, GL_FRAGMENT_SHADER)                                  \
    __SHADER(ANIM_FRAG, _embed_anim_frag, GL_FRAGMENT_SHADER)                                      \
    __SHADER(TEXT_FRAG, _embed_text_frag, GL_FRAGMENT_SHADER)                                      \
    __SHADER(UI_VERT, _embed_ui_vert, GL_VERTEX_SHADER)                                            \
    __SHADER(BILLBOARD_VERT, _embed_billboard_vert, GL_VERTEX_SHADER)

enum Shader {
#define __SHADER(label, str, type) label,
    __BUILTINSHADERS
#undef __SHADER
        SHADER_COUNT,
};

gpu::Shader *shader(builtin::Shader builtinShader);

} // namespace builtin