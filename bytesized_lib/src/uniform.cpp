#include "gpu.h"

#include "opengl.h"
#include <glm/gtc/type_ptr.hpp>

void gpu::Uniform::update() {
    switch (type) {
    case INTEGER:
        this->operator<<(i);
        break;
    case FLOAT:
        this->operator<<(f);
        break;
    case VEC2:
        this->operator<<(v2);
        break;
    case VEC3:
        this->operator<<(v3);
        break;
    case VEC4:
        this->operator<<(v4);
        break;
    case MAT3:
        this->operator<<(m3);
        break;
    case MAT4:
        this->operator<<(m4);
        break;
    case IVEC2:
        this->operator<<(iv2);
        break;
    case UNDEFINED:
        break;
    }
}

gpu::Uniform &gpu::Uniform::operator=(const int &value) {
    i = value;
    return this->operator<<(value);
}

gpu::Uniform &gpu::Uniform::operator=(const float &value) {
    f = value;
    return this->operator<<(value);
}

gpu::Uniform &gpu::Uniform::operator=(const glm::vec2 &value) {
    v2 = value;
    return this->operator<<(value);
}

gpu::Uniform &gpu::Uniform::operator=(const glm::vec3 &value) {
    v3 = value;
    return this->operator<<(value);
}

gpu::Uniform &gpu::Uniform::operator=(const glm::vec4 &value) {
    v4 = value;
    return this->operator<<(value);
}

gpu::Uniform &gpu::Uniform::operator=(const glm::mat3 &value) {
    m3 = value;
    return this->operator<<(value);
}

gpu::Uniform &gpu::Uniform::operator=(const glm::mat4 &value) {
    m4 = value;
    return this->operator<<(value);
}

gpu::Uniform &gpu::Uniform::operator=(const glm::ivec2 &value) {
    iv2 = value;
    return this->operator<<(value);
}

gpu::Uniform &gpu::Uniform::operator<<(const int &value) {
    glUniform1i(location, value);
    return *this;
}

gpu::Uniform &gpu::Uniform::operator<<(const float &value) {
    glUniform1f(location, value);
    return *this;
}

gpu::Uniform &gpu::Uniform::operator<<(const glm::vec2 &value) {
    glUniform2fv(location, 1, glm::value_ptr(value));
    return *this;
}

gpu::Uniform &gpu::Uniform::operator<<(const glm::vec3 &value) {
    glUniform3fv(location, 1, glm::value_ptr(value));
    return *this;
}

gpu::Uniform &gpu::Uniform::operator<<(const glm::vec4 &value) {
    glUniform4fv(location, 1, glm::value_ptr(value));
    return *this;
}

gpu::Uniform &gpu::Uniform::operator<<(const glm::mat3 &value) {
    glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
    return *this;
}

gpu::Uniform &gpu::Uniform::operator<<(const glm::mat4 &value) {
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    return *this;
}
gpu::Uniform &gpu::Uniform::operator<<(const glm::ivec2 &value) {
    glUniform2iv(location, 1, glm::value_ptr(value));
    return *this;
}