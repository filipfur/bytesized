#include "geom_primitive.h"
#include "primer.h"

geom::Sphere &geom::Sphere::construct(const glm::vec3 &center, float radii) {
    this->_center = center;
    this->_radii = radii;
    return *this;
}
geom::Sphere &geom::Sphere::construct(const glm::vec3 *points, size_t length, TRS *trs) {
    auto [min, max] = primer::minMaxOf(points, length, trs);
    auto [e, c] = primer::extentsCenter(min, max);
    return construct(c, std::max(e.x, std::max(e.y, e.z)));
}
bool geom::Sphere::_intersects(geom::Plane &other) { return other._intersects(*this); }
bool geom::Sphere::_intersects(geom::Sphere &other) {
    glm::vec3 d = other.origin() - this->origin();
    float r = other.radii() + this->radii();
    return glm::dot(d, d) < (r * r);
}
bool geom::Sphere::_intersects(geom::AABB &other) { return other._intersects(*this); }
bool geom::Sphere::_intersects(geom::OBB &other) { return other._intersects(*this); }
std::pair<glm::vec3, float> geom::Sphere::_separation(Plane &other) {
    return other._separation(*this);
}
std::pair<glm::vec3, float> geom::Sphere::_separation(Sphere &other) {
    glm::vec3 AB = this->origin() - other.origin();
    float dist = glm::length(AB);
    return {glm::normalize(AB), dist - (other.radii() + this->radii())};
}
std::pair<glm::vec3, float> geom::Sphere::_separation(AABB &other) {
    return other._separation(*this);
}
std::pair<glm::vec3, float> geom::Sphere::_separation(OBB &other) {
    return other._separation(*this);
}
glm::vec3 geom::Sphere::origin() {
    if (trs) {
        return trs->t() + _center;
    }
    return _center;
}
glm::vec3 geom::Sphere::supportPoint(const glm::vec3 &D) { return origin() + D * radii(); }