#pragma once

#include "geom_convexhull.h"
#include "geom_primitive.h"
#include "halfedge.h"
#include <vector>

namespace geom {
struct Collision {
    glm::vec3 contactPoint;
    glm::vec3 normal;
    float penetrationDepth;
    glm::vec3 a;
    glm::vec3 b;
    glm::vec3 c;
};

enum class gjk_state { ERROR, INCREMENTING, COLLISION, NO_COLLISION };

bool collides(Geometry &a, Geometry &b, geom::Collision *collision);

bool gjk(Geometry &a, Geometry &b, std::vector<glm::vec3> &simplex,
         const glm::vec3 &initialDirection, geom::Collision *collision);

void gjk_init();

geom::gjk_state gjk_increment(Geometry &a, Geometry &b, std::vector<glm::vec3> &simplex,
                              const glm::vec3 &initialDirection, geom::Collision *collision);

glm::vec3 getSupportPointOfA(Geometry &shape, const glm::vec3 &minkowskiSP);
glm::vec3 getSupportPointOfB(Geometry &shape, const glm::vec3 &minkowskiSP);

bool epa(Geometry &a, Geometry &b, const std::vector<glm::vec3> &simplex,
         geom::Collision *collision);
bool epa_increment(Geometry &a, Geometry &b, geom::ConvexHull &ch, geom::Collision *collision);
} // namespace geom