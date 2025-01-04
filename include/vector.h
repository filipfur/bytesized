#pragma once

#include "color.h"
#include <glm/glm.hpp>

struct Vector {
    Vector(const glm::vec3 &O_, const glm::vec3 &Ray_, const Color &color_)
        : O{O_}, Ray{Ray_}, color{color_}, hidden{false} {}
    Vector(const glm::vec3 &O_) : Vector{O_, {0.0f, 1.0f, 0.0f}, Color::yellow} {}
    Vector() : Vector{{0.0f, 0.0f, 0.0f}} {}
    glm::vec3 O;
    glm::vec3 Ray;
    Color color;
    bool hidden;
};