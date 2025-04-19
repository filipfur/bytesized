#include "geom_primitive.h"
#include "primer.h"

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
    float r = glm::dot(other.extents(), glm::abs(this->normal));
    return glm::dot(this->normal, other.origin()) - this->distance <= r;
}
bool geom::Plane::_intersects(OBB &) { return false; }

std::pair<glm::vec3, float> geom::Plane::_separation(Plane &) { return {{0.0f, 0.0f, 0.0f}, 0.0f}; }
std::pair<glm::vec3, float> geom::Plane::_separation(Sphere &other) {
    return {this->normal, other.radii() - glm::dot(other.origin() - origin(), this->normal)};
}
std::pair<glm::vec3, float> geom::Plane::_separation(AABB &other) {
    float r = glm::dot(other.extents(), glm::abs(this->normal));
    return {this->normal, r - (glm::dot(this->normal, other.origin()) - this->distance)};
}
std::pair<glm::vec3, float> geom::Plane::_separation(OBB &) { return {{0.0f, 0.0f, 0.0f}, 0.0f}; }