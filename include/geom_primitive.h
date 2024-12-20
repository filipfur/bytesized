#pragma once

#include "trs.h"

#include <glm/glm.hpp>

namespace geom {

struct Shape {
    enum Type { POSITIVE };
    Type type{POSITIVE};
    Shape *parent;
    Shape *child;
};

struct AABB : public Shape {
    glm::vec3 min;
    glm::vec3 max;
};

AABB *createAABB(const glm::vec3 &min, const glm::vec3 &max);
bool intersects(TRS &trs_a, const Shape &shape_a, TRS &trs_b, const Shape &shape_b);
bool intersects(TRS &trs_a, const AABB &aabb_a, TRS &trs_b, const AABB &aabb_b);

} // namespace geom