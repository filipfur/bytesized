#include "shader.h"

#include "gpu.h"
#include "opengl.h"

static const int _charbuf_len = 1024;
static GLchar _charbuf[_charbuf_len] = {};

void gpu::ShaderProgram::use() { glUseProgram(id); }

gpu::Uniform *gpu::ShaderProgram::uniform(const char *key) {
    auto it = uniforms.find(key);
    if (it == uniforms.end()) {
        return nullptr;
    }
    return &it->second;
}

bool gpu::Shader_compile(uint32_t type, const gpu::Shader &shader, const char *src) {
    glShaderSource(shader.id, 1, &src, nullptr);
    glCompileShader(shader.id);
    GLint status{0};
    GLint length{0};
    glGetShaderiv(shader.id, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        glGetShaderiv(shader.id, GL_INFO_LOG_LENGTH, &length);
        GLint n = length < _charbuf_len ? length : _charbuf_len;
        glGetShaderInfoLog(shader.id, length, &n, _charbuf);
        printf("%.*s\n", n, _charbuf);
        return false;
    }
    return true;
}

void gpu::Shader_createProgram(ShaderProgram &prog) {
    prog.id = glCreateProgram();
    glAttachShader(prog.id, prog.vertex->id);
    glAttachShader(prog.id, prog.fragment->id);
    glLinkProgram(prog.id);
    GLint res = GL_FALSE;
    glGetProgramiv(prog.id, GL_LINK_STATUS, &res);
    if (!res) {
        glGetProgramInfoLog(prog.id, _charbuf_len, NULL, _charbuf);
        printf("%s\n", _charbuf);
        return;
    }
    prog.use();
    for (auto it = prog.uniforms.begin(); it != prog.uniforms.end(); ++it) {
        it->second.location = glGetUniformLocation(prog.id, it->first.c_str());
        it->second.update();
    }
}

gpu::Shader *gpu::builtinShader(gpu::BuiltinShader builtinShader) {
    static gpu::Shader *shaders[] = {
#define __SHADER(label, str, type) gpu::createShader(type, str),
        __BUILTINSHADERS
#undef __SHADER
        nullptr};
    return shaders[static_cast<int>(builtinShader)];
}