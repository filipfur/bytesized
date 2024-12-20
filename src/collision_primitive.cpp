#include "geom_primitive.h"

#include "recycler.hpp"
#include <cassert>

#define COLLISION__AABB_COUNT 50

static recycler<geom::AABB, COLLISION__AABB_COUNT> _aabbs;

geom::AABB *geom::createAABB(const glm::vec3 &min, const glm::vec3 &max) {
    geom::AABB *aabb = _aabbs.acquire();
    aabb->min = min;
    aabb->max = max;
    return aabb;
}

bool geom::intersects(TRS & /*trs_a*/, const Shape & /*shape_a*/, TRS & /*trs_b*/,
                      const Shape & /*shape_b*/) {
    assert(false); // unsupported intersection
    return false;
}

bool geom::intersects(TRS &trs_a, const AABB &aabb_a, TRS &trs_b, const AABB &aabb_b) {
    glm::vec3 min_a = trs_a.t() + aabb_a.min;
    glm::vec3 max_a = trs_a.t() + aabb_a.max;
    glm::vec3 min_b = trs_b.t() + aabb_b.min;
    glm::vec3 max_b = trs_b.t() + aabb_b.max;
    return min_a.x <= max_b.x && max_a.x >= min_b.x && min_a.y <= max_b.y && max_a.y >= min_b.y &&
           min_a.z <= max_b.z && max_a.z >= min_b.z;
}