#include "geom_primitive.h"

#include "primer.h"
#include "recycler.hpp"
#include <cassert>

#define COLLISION__PLANE_COUNT 20
#define COLLISION__SPHERE_COUNT 20
#define COLLISION__AABB_COUNT 20
#define COLLISION__OBB_COUNT 20

static recycler<geom::Plane, COLLISION__PLANE_COUNT> _planes;
static recycler<geom::Sphere, COLLISION__SPHERE_COUNT> _spheres;
static recycler<geom::AABB, COLLISION__AABB_COUNT> _aabbs;
static recycler<geom::OBB, COLLISION__OBB_COUNT> _obbs;

geom::Plane &geom::Plane::construct(const glm::vec3 &normal_, float distance_) {
    this->normal = normal_;
    this->distance = distance_;
    return *this;
}
geom::Plane &geom::Plane::construct(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c) {
    const glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
    return construct(n, glm::dot(n, a));
}
geom::Plane &geom::Plane::construct(const glm::vec3 *points, size_t length, TRS *trs) {
    assert(length > 2);
    if (trs) {
        return construct(primer::transformPoint(trs->model(), points[0]),
                         primer::transformPoint(trs->model(), points[1]),
                         primer::transformPoint(trs->model(), points[2]));
    }
    return construct(points[0], points[1], points[2]);
}

glm::vec3 geom::Plane::origin() { return normal * distance; }
glm::vec3 geom::Plane::supportPoint(const glm::vec3 &D) {
    float denom = glm::dot(normal, D);
    if (denom < FLT_EPSILON && denom > -FLT_EPSILON) {
        return normal * distance;
    }
    float t = distance / denom;
    return D * t;
}

bool geom::Plane::_intersects(Plane &other) {
    float d = glm::dot(this->normal, other.normal);
    if (d < 0) {
        return d > -1.0f + primer::EPSILON || primer::isNear(this->distance, -other.distance);
    }
    return d < 1.0f + primer::EPSILON || primer::isNear(this->distance, other.distance);
}
bool geom::Plane::_intersects(Sphere &other) {
    return glm::dot(other.origin() - origin(), this->normal) <= other.radii();
}
bool geom::Plane::_intersects(AABB &other) {
    glm::vec3 e = (other.max - other.min) * 0.5f;
    glm::vec3 c = other.min + e;
    float r = glm::dot(e, glm::abs(this->normal));
    return glm::dot(this->normal, c) - this->distance <= r;
}
bool geom::Plane::_intersects(OBB &other) { return false; }

std::pair<glm::vec3, float> geom::Plane::_separation(Plane &other) {
    return {{0.0f, 0.0f, 0.0f}, 0.0f};
}
std::pair<glm::vec3, float> geom::Plane::_separation(Sphere &other) {
    return {this->normal, other.radii() - glm::dot(other.origin() - origin(), this->normal)};
}
std::pair<glm::vec3, float> geom::Plane::_separation(AABB &other) {
    glm::vec3 e = (other.max - other.min) * 0.5f;
    glm::vec3 c = other.min + e;
    float r = glm::dot(e, glm::abs(this->normal));
    return {this->normal, r - (glm::dot(this->normal, c) - this->distance)};
}
std::pair<glm::vec3, float> geom::Plane::_separation(OBB &other) {
    return {{0.0f, 0.0f, 0.0f}, 0.0f};
}

// --------------------------------
geom::Sphere &geom::Sphere::construct(const glm::vec3 &center, float radii) {
    this->_center = center;
    this->_radii = radii;
    return *this;
}
geom::Sphere &geom::Sphere::construct(const glm::vec3 *points, size_t length, TRS *trs) {
    auto [min, max] = primer::minMaxOf(points, length, trs);
    glm::vec3 e = (max - min) * 0.5f;
    glm::vec3 c = min + e;
    return construct(c, std::max(e.x, std::max(e.y, e.z)));
}
bool geom::Sphere::_intersects(geom::Plane &other) { return other._intersects(*this); }
bool geom::Sphere::_intersects(geom::AABB &other) { return other._intersects(*this); }
std::pair<glm::vec3, float> geom::Sphere::_separation(geom::Plane &other) {
    return other._separation(*this);
}
std::pair<glm::vec3, float> geom::Sphere::_separation(geom::AABB &other) {
    return other._separation(*this);
}
glm::vec3 geom::Sphere::origin() {
    if (trs) {
        return trs->t() + _center;
    }
    return _center;
}
glm::vec3 geom::Sphere::supportPoint(const glm::vec3 &D) { return origin() + D * radii(); }
// --------------------------------
geom::AABB &geom::AABB::construct(const glm::vec3 &min_, const glm::vec3 &max_) {
    this->min = min_;
    this->max = max_;
    return *this;
}

geom::AABB &geom::AABB::construct(const glm::vec3 *points, size_t length, TRS *trs) {
    auto [min_, max_] = primer::minMaxOf(points, length, trs);
    for (size_t i{0}; i < 3; ++i) {
        if (primer::isNear(min_[i], max_[i])) {
            min_[i] -= 0.001f;
            max_[i] += 0.001f;
        }
    }
    return construct(min_, max_);
}

glm::vec3 geom::AABB::origin() { return (min + max) * 0.5f; }
glm::vec3 geom::AABB::supportPoint(const glm::vec3 &D) {
    const glm::vec3 he = (max - min) * 0.5f;
    const glm::vec3 O = min + he;
    return {
        O.x + (D.x > 0 ? he.x : -he.x),
        O.y + (D.y > 0 ? he.y : -he.y),
        O.z + (D.z > 0 ? he.z : -he.z),
    };
}

bool geom::AABB::_intersects(Plane &other) { return other._intersects(*this); }
bool geom::AABB::_intersects(Sphere &other) {
    glm::vec3 e = (this->max - this->min) * 0.5f;
    glm::vec3 a = this->min + e;
    const glm::vec3 b = other.origin();
    const glm::vec3 ab = b - a;
    float maxDist = 0.0f;
    size_t ax = 0;
    for (size_t i{0}; i < 3; ++i) {
        float d = glm::abs(ab[i]);
        if (d > maxDist) {
            maxDist = d;
            ax = i;
        }
    }
    /*glm::vec3 n{0.0f, 0.0f, 0.0f};
    n[ax] = ab[ax] < 0 ? -1.0f : 1.0f;*/
    return other.radii() + e[ax] >= glm::abs(ab[ax]);
}
bool geom::AABB::_intersects(AABB &other) {
    return this->min.x <= other.max.x && this->max.x >= other.min.x && // br
           this->min.y <= other.max.y && this->max.y >= other.min.y && // br
           this->min.z <= other.max.z && this->max.z >= other.min.z;
}

std::pair<glm::vec3, float> geom::AABB::_separation(Plane &other) {
    return other._separation(*this);
}
std::pair<glm::vec3, float> geom::AABB::_separation(Sphere &other) {
    glm::vec3 e = (this->max - this->min) * 0.5f;
    glm::vec3 a = this->min + e;
    const glm::vec3 &b = other.origin();
    const glm::vec3 ab = b - a;
    float maxDist = 0.0f;
    size_t ax = 0;
    for (size_t i{0}; i < 3; ++i) {
        float d = glm::abs(ab[i]);
        if (d > maxDist) {
            maxDist = d;
            ax = i;
        }
    }
    float dist = other.radii() + e[ax] - glm::abs(ab[ax]);
    glm::vec3 n{0.0f, 0.0f, 0.0f};
    n[ax] = ab[ax] < 0 ? -1.0f : 1.0f;
    // print_glm(n);
    // printf("dist=%f\n", dist);
    return {n, dist};
}
std::pair<glm::vec3, float> geom::AABB::_separation(AABB &other) {
    float minDist{FLT_MAX};
    size_t ax{0};
    for (size_t i{0}; i < 3; ++i) {
        const glm::vec3 &axis = primer::AXES[i];
        float ax2 = glm::dot(axis, axis);
        float d0 = (other.max[i] - this->min[i]);
        float d1 = (this->max[i] - other.min[i]);
        if (d0 <= 0.0f || d1 <= 0.0f) {
            return {{0.0f, 0.0f, 0.0f}, 0.0f};
        }
        float overlap = (d0 < d1) ? d0 : -d1;
        glm::vec3 sep = axis * (overlap / ax2);
        float sepl2 = glm::dot(sep, sep);
        if (sepl2 < minDist) {
            minDist = sepl2;
            ax = i;
        }
    }
    return {primer::AXES[ax], glm::sqrt(minDist)};
}
// --------------------------------
geom::OBB &geom::OBB::construct(const glm::vec3 &center, const glm::vec3 &extents, TRS *trs) {
    this->c = center;
    this->trs = trs;
    this->e = extents;
    return *this;
}
geom::OBB &geom::OBB::construct(const glm::vec3 *points, size_t length, TRS *trs) {
    auto [min, max] = primer::minMaxOf(points, length, nullptr);
    glm::vec3 e_ = (max - min) * 0.5f;
    glm::vec3 c_ = min + e;
    return construct(c_, e_, trs);
}
bool geom::OBB::_intersects(geom::Plane &other) { return false; }
bool geom::OBB::_intersects(geom::OBB &other) {
    geom::OBB &a = *this;
    geom::OBB &b = other;
    const glm::mat4 &mA = a.trs->model();
    const glm::mat4 &mB = b.trs->model();
    glm::vec3 u_a[3] = {
        glm::normalize(glm::vec3(mA[0])),
        glm::normalize(glm::vec3(mA[1])),
        glm::normalize(glm::vec3(mA[2])),
    };
    glm::vec3 u_b[3] = {
        glm::normalize(glm::vec3(mB[0])),
        glm::normalize(glm::vec3(mB[1])),
        glm::normalize(glm::vec3(mB[2])),
    };
    glm::vec3 a_e = a.e * a.trs->s();
    glm::vec3 b_e = b.e * b.trs->s();
    float ra, rb;
    static glm::mat3 R, AbsR;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            R[i][j] = glm::dot(glm::normalize(u_a[i]), glm::normalize(u_b[j]));
        }
    }
    // Compute translation vector t
    glm::vec3 t = b.origin() - a.origin();
    // Bring translation into a’s coordinate frame
    t = glm::vec3(glm::dot(t, u_a[0]), glm::dot(t, u_a[1]), glm::dot(t, u_a[2]));

    // Compute common subexpressions. Add in an epsilon term to
    // counteract arithmetic errors when two edges are parallel and // their cross product is (near)
    // null (see text for details) for (int i = 0; i < 3; i++)
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            AbsR[i][j] = glm::abs(R[i][j]) + FLT_EPSILON;
        }
    }

    float d;

    // Test axes L = A0, L = A1, L = A2
    for (int i = 0; i < 3; i++) {
        ra = a_e[i];
        rb = b_e[0] * AbsR[i][0] + b_e[1] * AbsR[i][1] + b_e[2] * AbsR[i][2];
        d = glm::abs(t[i]);
        if (d > ra + rb)
            return false;
    }
    // Test axes L = B0, L = B1, L = B2
    for (int i = 0; i < 3; i++) {
        ra = a_e[0] * AbsR[0][i] + a_e[1] * AbsR[1][i] + a_e[2] * AbsR[2][i];
        rb = b_e[i];
        d = glm::abs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]);
        if (d > ra + rb)
            return false;
    }
    // Test axis L = A0 x B0
    ra = a_e[1] * AbsR[2][0] + a_e[2] * AbsR[1][0];
    rb = b_e[1] * AbsR[0][2] + b_e[2] * AbsR[0][1];
    d = glm::abs(t[2] * R[1][0] - t[1] * R[2][0]);
    if (d > ra + rb)
        return false;
    // Test axis L = A0 x B1
    ra = a_e[1] * AbsR[2][1] + a_e[2] * AbsR[1][1];
    rb = b_e[0] * AbsR[0][2] + b_e[2] * AbsR[0][0];
    d = glm::abs(t[2] * R[1][1] - t[1] * R[2][1]);
    if (d > ra + rb)
        return false;
    // Test axis L = A0 x B2
    ra = a_e[1] * AbsR[2][2] + a_e[2] * AbsR[1][2];
    rb = b_e[0] * AbsR[0][1] + b_e[1] * AbsR[0][0];
    d = glm::abs(t[2] * R[1][2] - t[1] * R[2][2]);
    if (d > ra + rb)
        return false;
    // Test axis L = A1 x B0
    ra = a_e[0] * AbsR[2][0] + a_e[2] * AbsR[0][0];
    rb = b_e[1] * AbsR[1][2] + b_e[2] * AbsR[1][1];
    d = glm::abs(t[0] * R[2][0] - t[2] * R[0][0]);
    if (d > ra + rb)
        return false;
    // Test axis L = A1 x B1
    ra = a_e[0] * AbsR[2][1] + a_e[2] * AbsR[0][1];
    rb = b_e[0] * AbsR[1][2] + b_e[2] * AbsR[1][0];
    d = glm::abs(t[0] * R[2][1] - t[2] * R[0][1]);
    if (d > ra + rb)
        return false;
    // Test axis L = A1 x B2
    ra = a_e[0] * AbsR[2][2] + a_e[2] * AbsR[0][2];
    rb = b_e[0] * AbsR[1][1] + b_e[1] * AbsR[1][0];
    d = glm::abs(t[0] * R[2][2] - t[2] * R[0][2]);
    if (d > ra + rb)
        return false;
    // Test axis L = A2 x B0
    ra = a_e[0] * AbsR[1][0] + a_e[1] * AbsR[0][0];
    rb = b_e[1] * AbsR[2][2] + b_e[2] * AbsR[2][1];
    d = glm::abs(t[1] * R[0][0] - t[0] * R[1][0]);
    if (d > ra + rb)
        return false;
    // Test axis L = A2 x B1
    ra = a_e[0] * AbsR[1][1] + a_e[1] * AbsR[0][1];
    rb = b_e[0] * AbsR[2][2] + b_e[2] * AbsR[2][0];
    d = glm::abs(t[1] * R[0][1] - t[0] * R[1][1]);
    if (d > ra + rb)
        return false;
    // Test axis L = A2 x B2
    ra = a_e[0] * AbsR[1][2] + a_e[1] * AbsR[0][2];
    rb = b_e[0] * AbsR[2][1] + b_e[1] * AbsR[2][0];
    d = glm::abs(t[1] * R[0][2] - t[0] * R[1][2]);
    if (d > ra + rb)
        return false;
    // Since no separating axis is found, the OBBs must be intersecting
    return true;
}

std::pair<glm::vec3, float> geom::OBB::_separation(geom::Plane &other) {
    return {{0.0f, 0.0f, 0.0f}, 0.0f};
}
std::pair<glm::vec3, float> geom::OBB::_separation(geom::OBB &other) {
    geom::OBB &a = *this;
    geom::OBB &b = other;
    const glm::mat4 &mA = a.trs->model();
    const glm::mat4 &mB = b.trs->model();
    glm::vec3 u_a[3] = {
        glm::normalize(glm::vec3(mA[0])),
        glm::normalize(glm::vec3(mA[1])),
        glm::normalize(glm::vec3(mA[2])),
    };
    glm::vec3 u_b[3] = {
        glm::normalize(glm::vec3(mB[0])),
        glm::normalize(glm::vec3(mB[1])),
        glm::normalize(glm::vec3(mB[2])),
    };
    glm::vec3 a_e = a.e * a.trs->s();
    glm::vec3 b_e = b.e * b.trs->s();
    float ra, rb;
    static glm::mat3 R, AbsR;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            R[i][j] = glm::dot(glm::normalize(u_a[i]), glm::normalize(u_b[j]));
        }
    }
    // Compute translation vector t
    glm::vec3 t = b.origin() - a.origin();
    // Bring translation into a’s coordinate frame
    t = glm::vec3(glm::dot(t, u_a[0]), glm::dot(t, u_a[1]), glm::dot(t, u_a[2]));

    // Compute common subexpressions. Add in an epsilon term to
    // counteract arithmetic errors when two edges are parallel and // their cross product is (near)
    // null (see text for details) for (int i = 0; i < 3; i++)
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            AbsR[i][j] = glm::abs(R[i][j]) + FLT_EPSILON;
        }
    }

    float minDist = FLT_MAX;
    glm::vec3 minAxis{0.0f, 0.0f, 0.0f};
    float d;
    float dist;

    // Test axes L = A0, L = A1, L = A2
    for (int i = 0; i < 3; i++) {
        ra = a_e[i];
        rb = b_e[0] * AbsR[i][0] + b_e[1] * AbsR[i][1] + b_e[2] * AbsR[i][2];
        d = glm::abs(t[i]);
        if (d > ra + rb)
            return {{0.0f, 0.0f, 0.0f}, 0.0f};
        dist = (ra + rb) - d;
        if (dist < minDist && dist > 0) {
            minDist = dist;
            minAxis = u_a[i];
        }
    }
    // Test axes L = B0, L = B1, L = B2
    for (int i = 0; i < 3; i++) {
        ra = a_e[0] * AbsR[0][i] + a_e[1] * AbsR[1][i] + a_e[2] * AbsR[2][i];
        rb = b_e[i];
        d = glm::abs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]);
        if (d > ra + rb)
            return {{0.0f, 0.0f, 0.0f}, 0.0f};
        dist = (ra + rb) - d;
        if (dist < minDist && dist > 0) {
            minDist = dist;
            minAxis = u_b[i];
        }
    }
    // Test axis L = A0 x B0
    ra = a_e[1] * AbsR[2][0] + a_e[2] * AbsR[1][0];
    rb = b_e[1] * AbsR[0][2] + b_e[2] * AbsR[0][1];
    d = glm::abs(t[2] * R[1][0] - t[1] * R[2][0]);
    if (d > ra + rb)
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    dist = (ra + rb) - d;
    if (dist < minDist && dist > 0) {
        glm::vec3 n = glm::cross(u_a[0], u_b[0]);
        if (glm::dot(n, n) > primer::EPSILON) {
            minDist = dist;
            minAxis = glm::normalize(n);
        }
    }
    // Test axis L = A0 x B1
    ra = a_e[1] * AbsR[2][1] + a_e[2] * AbsR[1][1];
    rb = b_e[0] * AbsR[0][2] + b_e[2] * AbsR[0][0];
    d = glm::abs(t[2] * R[1][1] - t[1] * R[2][1]);
    if (d > ra + rb)
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    dist = (ra + rb) - d;
    if (dist < minDist && dist > 0) {
        glm::vec3 n = glm::cross(u_a[0], u_b[1]);
        if (glm::dot(n, n) > primer::EPSILON) {
            minDist = dist;
            minAxis = glm::normalize(n);
        }
    }
    // Test axis L = A0 x B2
    ra = a_e[1] * AbsR[2][2] + a_e[2] * AbsR[1][2];
    rb = b_e[0] * AbsR[0][1] + b_e[1] * AbsR[0][0];
    d = glm::abs(t[2] * R[1][2] - t[1] * R[2][2]);
    if (d > ra + rb)
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    dist = (ra + rb) - d;
    if (dist < minDist && dist > 0) {
        glm::vec3 n = glm::cross(u_a[0], u_b[2]);
        if (glm::dot(n, n) > primer::EPSILON) {
            minDist = dist;
            minAxis = glm::normalize(n);
        }
    }
    // Test axis L = A1 x B0
    ra = a_e[0] * AbsR[2][0] + a_e[2] * AbsR[0][0];
    rb = b_e[1] * AbsR[1][2] + b_e[2] * AbsR[1][1];
    d = glm::abs(t[0] * R[2][0] - t[2] * R[0][0]);
    if (d > ra + rb)
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    dist = (ra + rb) - d;
    if (dist < minDist && dist > 0) {
        glm::vec3 n = glm::cross(u_a[1], u_b[0]);
        if (glm::dot(n, n) > primer::EPSILON) {
            minDist = dist;
            minAxis = glm::normalize(n);
        }
    }
    // Test axis L = A1 x B1
    ra = a_e[0] * AbsR[2][1] + a_e[2] * AbsR[0][1];
    rb = b_e[0] * AbsR[1][2] + b_e[2] * AbsR[1][0];
    d = glm::abs(t[0] * R[2][1] - t[2] * R[0][1]);
    if (d > ra + rb)
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    dist = (ra + rb) - d;
    if (dist < minDist && dist > 0) {
        glm::vec3 n = glm::cross(u_a[1], u_b[1]);
        if (glm::dot(n, n) > primer::EPSILON) {
            minDist = dist;
            minAxis = glm::normalize(n);
        }
    }
    // Test axis L = A1 x B2
    ra = a_e[0] * AbsR[2][2] + a_e[2] * AbsR[0][2];
    rb = b_e[0] * AbsR[1][1] + b_e[1] * AbsR[1][0];
    d = glm::abs(t[0] * R[2][2] - t[2] * R[0][2]);
    if (d > ra + rb)
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    dist = (ra + rb) - d;
    if (dist < minDist && dist > 0) {
        glm::vec3 n = glm::cross(u_a[1], u_b[2]);
        if (glm::dot(n, n) > primer::EPSILON) {
            minDist = dist;
            minAxis = glm::normalize(n);
        }
    }
    // Test axis L = A2 x B0
    ra = a_e[0] * AbsR[1][0] + a_e[1] * AbsR[0][0];
    rb = b_e[1] * AbsR[2][2] + b_e[2] * AbsR[2][1];
    d = glm::abs(t[1] * R[0][0] - t[0] * R[1][0]);
    if (d > ra + rb)
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    dist = (ra + rb) - d;
    if (dist < minDist && dist > 0) {
        glm::vec3 n = glm::cross(u_a[2], u_b[0]);
        if (glm::dot(n, n) > primer::EPSILON) {
            minDist = dist;
            minAxis = glm::normalize(n);
        }
    }
    // Test axis L = A2 x B1
    ra = a_e[0] * AbsR[1][1] + a_e[1] * AbsR[0][1];
    rb = b_e[0] * AbsR[2][2] + b_e[2] * AbsR[2][0];
    d = glm::abs(t[1] * R[0][1] - t[0] * R[1][1]);
    if (d > ra + rb)
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    dist = (ra + rb) - d;
    if (dist < minDist && dist > 0) {
        glm::vec3 n = glm::cross(u_a[2], u_b[1]);
        if (glm::dot(n, n) > primer::EPSILON) {
            minDist = dist;
            minAxis = glm::normalize(n);
        }
    }
    // Test axis L = A2 x B2
    ra = a_e[0] * AbsR[1][2] + a_e[1] * AbsR[0][2];
    rb = b_e[0] * AbsR[2][1] + b_e[1] * AbsR[2][0];
    d = glm::abs(t[1] * R[0][2] - t[0] * R[1][2]);
    if (d > ra + rb)
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    dist = (ra + rb) - d;
    if (dist < minDist && dist > 0) {
        glm::vec3 n = glm::cross(u_a[2], u_b[2]);
        if (glm::dot(n, n) > primer::EPSILON) {
            minDist = dist;
            minAxis = glm::normalize(n);
        }
    }
    // Since no separating axis is found, the OBBs must be intersecting
    // glm::vec3 n = glm::normalize(minAxis);
    // print_glm(minDist);
    assert(!glm::isnan(minAxis.x) && !glm::isnan(minAxis.y) && !glm::isnan(minAxis.z));
    return {minAxis, minDist};
}

glm::vec3 geom::OBB::origin() { return trs->t() + c; }
glm::vec3 geom::OBB::supportPoint(const glm::vec3 &D) {
    glm::vec3 d = glm::mat3(trs->model()) * D;
    return {
        c.x + (d.x > 0 ? e.x : -e.x),
        c.y + (d.y > 0 ? e.y : -e.y),
        c.z + (d.z > 0 ? e.z : -e.z),
    };
}
// --------------------------------

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
