#pragma once

#include <glm/glm.hpp>

namespace gpu {
class Uniform {
  public:
    Uniform() : type{UNDEFINED} {}
    Uniform(const int &value) : type{INTEGER}, i{value} {}
    Uniform(const float &value) : type{FLOAT}, f{value} {}
    Uniform(const glm::vec2 &value) : type{VEC2}, v2{value} {}
    Uniform(const glm::vec3 &value) : type{VEC3}, v3{value} {}
    Uniform(const glm::vec4 &value) : type{VEC4}, v4{value} {}
    Uniform(const glm::mat3 &value) : type{MAT3}, m3{value} {}
    Uniform(const glm::mat4 &value) : type{MAT4}, m4{value} {}

    Uniform &operator=(const int &value);
    Uniform &operator=(const float &value);
    Uniform &operator=(const glm::vec2 &value);
    Uniform &operator=(const glm::vec3 &value);
    Uniform &operator=(const glm::vec4 &value);
    Uniform &operator=(const glm::mat3 &value);
    Uniform &operator=(const glm::mat4 &value);

    gpu::Uniform &operator<<(const int &value);
    gpu::Uniform &operator<<(const float &value);
    gpu::Uniform &operator<<(const glm::vec2 &value);
    gpu::Uniform &operator<<(const glm::vec3 &value);
    gpu::Uniform &operator<<(const glm::vec4 &value);
    gpu::Uniform &operator<<(const glm::mat3 &value);
    gpu::Uniform &operator<<(const glm::mat4 &value);

    void update();

    int location;

  private:
    enum Type { UNDEFINED, INTEGER, FLOAT, VEC2, VEC3, VEC4, MAT3, MAT4 } type;
    union {
        int i;
        float f;
        glm::vec2 v2;
        glm::vec3 v3;
        glm::vec4 v4;
        glm::mat3 m3;
        glm::mat4 m4;
    };
};
} // namespace gpu