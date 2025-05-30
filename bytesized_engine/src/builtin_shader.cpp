#include "builtin_shader.h"

#include "gpu.h"

gpu::Shader *builtin::shader(builtin::Shader builtinShader) {
    static gpu::Shader *shaders[] = {
#define __SHADER(label, str, type) gpu::createShader(type, (const char *)str),
        __BUILTINSHADERS
#undef __SHADER
        nullptr};
    return shaders[static_cast<int>(builtinShader)];
}