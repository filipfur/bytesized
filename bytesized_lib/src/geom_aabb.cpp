#include "geom_primitive.h"
#include "primer.h"

glm::vec3 geom::_closestPointAABB(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &e) {
    glm::vec3 X;
    glm::vec3 min = a - e;
    glm::vec3 max = a + e;
    for (size_t i{0}; i < 3; ++i) {
        float x = p[i];
        if (x < min[i])
            x = min[i];
        if (x > max[i])
            x = max[i];
        X[i] = x;
        // std::max(std::min(x, min[i]), max[i]);
    }
    return X;
}

static bool _aabbSphereIntersects(const glm::vec3 &a, const glm::vec3 &e, const glm::vec3 &b,
                                  float r) {
    for (size_t i{0}; i < 3; ++i) {
        float l0 = b[i] - r; //  0.9 -2.9
        float l1 = b[i] + r; //  2.9 -0.9

        float l2 = a[i] - e[i]; // -1.0
        float l3 = a[i] + e[i]; // 1.0

        float d0 = l0 - l3; // -0.1
        float d1 = l2 - l1; // -3.9

        if ((d0 > 0) != (d1 > 0)) {
            return false;
        }
    }
    return true;
}

static std::pair<glm::vec3, float> _aabbSphereSeparation(const glm::vec3 &a, const glm::vec3 &e,
                                                         const glm::vec3 &b, float r) {
    const glm::vec3 ab = b - a;
    float minDist = FLT_MAX;
    size_t ax = 0;

    for (size_t i{0}; i < 3; ++i) {
        float l0 = b[i] - r;
        float l1 = b[i] + r;

        float l2 = a[i] - e[i];
        float l3 = a[i] + e[i];

        float d0 = l0 - l3; // -0.1
        float d1 = l2 - l1; // -3.9

        if ((d0 > 0) == (d1 > 0)) {
            float d = glm::min(glm::abs(d0), glm::abs(d1));
            if (d < minDist) {
                minDist = d;
                ax = i;
            }
        }
    }
    if (minDist == FLT_MAX) {
        glm::vec3 n = glm::normalize(ab);
        return {n, glm::length(ab) - glm::dot(n, e) - r};
    }
    glm::vec3 n{0.0f, 0.0f, 0.0f};
    n[ax] = ab[ax] < 0 ? -1.0f : 1.0f;
    return {n, minDist};
}

geom::AABB &geom::AABB::construct(const glm::vec3 &min, const glm::vec3 &max) {
    auto [e, c] = primer::extentsCenter(min, max);
    _extents = e;
    _center = c;
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

glm::vec3 geom::AABB::extents() const {
    if (trs) {
        return _extents * trs->s();
    }
    return _extents;
}

std::pair<glm::vec3, glm::vec3> geom::AABB::minMax() const {
    if (trs) {
        glm::vec3 c = trs->t() + _center;
        glm::vec3 e = extents();
        return {c - e, c + e};
    }
    return {_center - _extents, _center + _extents};
}

glm::vec3 geom::AABB::origin() {
    if (trs) {
        return trs->t() + _center;
    }
    return _center;
}

glm::vec3 geom::AABB::supportPoint(const glm::vec3 &D) {
    const glm::vec3 c = origin();
    const glm::vec3 e = extents();
    return {
        c.x + (D.x > 0 ? e.x : -e.x),
        c.y + (D.y > 0 ? e.y : -e.y),
        c.z + (D.z > 0 ? e.z : -e.z),
    };
}

bool geom::AABB::_intersects(Plane &other) { return other._intersects(*this); }

bool geom::AABB::_intersects(Sphere &other) {
    return _aabbSphereIntersects(origin(), extents(), other.origin(), other.radii());
}
bool geom::AABB::_intersects(AABB &other) {
    auto [min_a, max_a] = this->minMax();
    auto [min_b, max_b] = other.minMax();
    return min_a.x <= max_b.x && max_a.x >= min_b.x && // br
           min_a.y <= max_b.y && max_a.y >= min_b.y && // br
           min_a.z <= max_b.z && max_a.z >= min_b.z;
}

bool geom::AABB::rayIntersect(const glm::vec3 &p, const glm::vec3 &d, float &tmin, float &tmax) {

    auto [min, max] = minMax();
    for (size_t i{0}; i < 3; ++i) {
        if (glm::abs(d[i]) < primer::EPSILON) {
            // Ray is parallel to slab. No hit if origin not within slab
            if (p[i] < min[i] || p[i] > max[i])
                return false;
        } else {
            // Compute intersection t value of ray with near and far plane of slab
            float ood = 1.0f / d[i];
            float t1 = (min[i] - p[i]) * ood;
            float t2 = (max[i] - p[i]) * ood;
            // Make t1 be intersection with near plane, t2 with far plane
            if (t1 > t2)
                std::swap(t1, t2);
            // Compute the intersection of slab intersection intervals
            if (t1 > tmin)
                tmin = t1;
            if (t2 < tmax)
                tmax = t2;
            // Exit with no collision as soon as slab intersection becomes empty
            if (tmin > tmax)
                return false;
        }
    }
    return true;
}

std::pair<glm::vec3, float> geom::AABB::_separation(Plane &other) {
    return other._separation(*this);
}

std::pair<glm::vec3, float> geom::AABB::_separation(Sphere &other) {
    return _aabbSphereSeparation(origin(), extents(), other.origin(), other.radii());
}
std::pair<glm::vec3, float> geom::AABB::_separation(AABB &other) {
    float minDist{FLT_MAX};
    size_t ax{0};
    for (size_t i{0}; i < 3; ++i) {
        const glm::vec3 &axis = primer::AXES[i];
        float ax2 = glm::dot(axis, axis);
        auto [min_a, max_a] = this->minMax();
        auto [min_b, max_b] = other.minMax();
        float d0 = (max_b[i] - min_a[i]);
        float d1 = (max_a[i] - min_b[i]);
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