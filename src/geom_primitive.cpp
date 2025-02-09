#include "geom_primitive.h"

#include "primer.h"
#include "recycler.hpp"
#include <cassert>

#define COLLISION__PLANE_COUNT 20
#define COLLISION__SPHERE_COUNT 20
#define COLLISION__AABB_COUNT 50
#define COLLISION__OBB_COUNT 20

static recycler<geom::Plane, COLLISION__PLANE_COUNT> _planes;
static recycler<geom::Sphere, COLLISION__SPHERE_COUNT> _spheres;
static recycler<geom::AABB, COLLISION__AABB_COUNT> _aabbs;
static recycler<geom::OBB, COLLISION__OBB_COUNT> _obbs;

geom::Plane *geom::createPlane() {
    geom::Plane *plane = _planes.acquire();
    return plane;
}

geom::Sphere *geom::createSphere() {
    geom::Sphere *sphere = _spheres.acquire();
    return sphere;
}

geom::AABB *geom::createAABB() {
    geom::AABB *aabb = _aabbs.acquire();
    return aabb;
}

geom::OBB *geom::createOBB() {
    geom::OBB *obb = _obbs.acquire();
    return obb;
}
