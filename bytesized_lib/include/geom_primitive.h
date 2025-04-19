#pragma once

#include "trs.h"

#include <glm/glm.hpp>

// #pragma clang diagnostic push
// #pragma clang diagnostic ignored "-Wunused-parameter"

namespace geom {

glm::vec3 _closestPointAABB(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &e);
glm::vec3 _closestPointOBB(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &e,
                           const glm::mat4 &m);

struct Geometry {
    enum class Type { PLANE, SPHERE, AABB, OBB };

    Geometry() = default;

    virtual ~Geometry() noexcept {}

    virtual Geometry &construct(const glm::vec3 *points, size_t length, TRS *trs) = 0;

    Geometry *parent;
    Geometry *child;
    TRS *trs;
    virtual Type type() = 0;
    virtual glm::vec3 origin() = 0;
    virtual glm::vec3 supportPoint(const glm::vec3 &D) = 0;

    virtual bool isTrivialIntersect(Geometry &) = 0;
    virtual bool _isTrivialIntersect(struct Plane &) { return false; }
    virtual bool _isTrivialIntersect(struct Sphere &) { return false; }
    virtual bool _isTrivialIntersect(struct AABB &) { return false; }
    virtual bool _isTrivialIntersect(struct OBB &) { return false; }

    virtual bool intersects(Geometry &) = 0;
    virtual bool _intersects(struct Plane &) { return false; }
    virtual bool _intersects(struct Sphere &) { return false; }
    virtual bool _intersects(struct AABB &) { return false; }
    virtual bool _intersects(struct OBB &) { return false; }

    virtual std::pair<glm::vec3, float> separation(Geometry &other) = 0;
    virtual std::pair<glm::vec3, float> _separation(struct Plane &) {
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    }
    virtual std::pair<glm::vec3, float> _separation(struct Sphere &) {
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    }
    virtual std::pair<glm::vec3, float> _separation(struct AABB &) {
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    }
    virtual std::pair<glm::vec3, float> _separation(struct OBB &) {
        return {{0.0f, 0.0f, 0.0f}, 0.0f};
    }

    virtual bool rayIntersect(const glm::vec3 &p, const glm::vec3 &d, float &tmin, float &tmax) = 0;
};

struct Plane : public Geometry {
    Plane() = default;
    Plane &construct(const glm::vec3 &normal, float distance);
    Plane &construct(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c);
    Plane &construct(const glm::vec3 *points, size_t length, TRS *trs) override;

    virtual ~Plane() noexcept {}

    glm::vec3 normal;
    float distance; // from the origin
    Type type() override { return Type::PLANE; };
    bool isTrivialIntersect(Geometry &other) override { return other._isTrivialIntersect(*this); }
    bool _isTrivialIntersect(struct Plane &) override { return true; }
    bool _isTrivialIntersect(struct Sphere &) override { return true; }
    bool _isTrivialIntersect(struct AABB &) override { return true; }
    bool _isTrivialIntersect(struct OBB &) override { return true; }
    bool intersects(geom::Geometry &other) override { return other._intersects(*this); }
    bool _intersects(struct Plane &other) override;
    bool _intersects(struct Sphere &other) override;
    bool _intersects(struct AABB &other) override;
    bool _intersects(struct OBB &other) override;
    bool rayIntersect(const glm::vec3 &, const glm::vec3 &, float &, float &) override {
        return false;
    }
    std::pair<glm::vec3, float> separation(Geometry &other) override {
        return other._separation(*this);
    }
    std::pair<glm::vec3, float> _separation(struct Plane &other) override;
    std::pair<glm::vec3, float> _separation(struct Sphere &other) override;
    std::pair<glm::vec3, float> _separation(struct AABB &other) override;
    std::pair<glm::vec3, float> _separation(struct OBB &other) override;
    glm::vec3 origin() override;
    glm::vec3 supportPoint(const glm::vec3 &D) override;
};

struct Sphere : public Geometry {
    Sphere() = default;
    Sphere &construct(const glm::vec3 &center, float radii);
    Sphere &construct(const glm::vec3 *points, size_t length, TRS *trs) override;

    virtual ~Sphere() noexcept {}

    Type type() override { return Type::SPHERE; }
    bool isTrivialIntersect(Geometry &other) override { return other._isTrivialIntersect(*this); }
    bool _isTrivialIntersect(struct Plane &) override { return true; }
    bool _isTrivialIntersect(struct Sphere &) override { return true; }
    bool _isTrivialIntersect(struct AABB &) override { return true; }
    bool _isTrivialIntersect(struct OBB &) override { return true; }
    bool intersects(geom::Geometry &other) override { return other._intersects(*this); }
    bool _intersects(struct Plane &other) override;
    bool _intersects(struct Sphere &other) override;
    bool _intersects(struct AABB &other) override;
    bool _intersects(struct OBB &other) override;
    bool rayIntersect(const glm::vec3 &, const glm::vec3 &, float &, float &) override {
        return false;
    }
    std::pair<glm::vec3, float> separation(Geometry &other) override {
        return other._separation(*this);
    }
    std::pair<glm::vec3, float> _separation(struct Plane &other) override;
    std::pair<glm::vec3, float> _separation(struct Sphere &other) override;
    std::pair<glm::vec3, float> _separation(struct AABB &other) override;
    std::pair<glm::vec3, float> _separation(struct OBB &other) override;
    glm::vec3 origin() override;
    glm::vec3 supportPoint(const glm::vec3 &D) override;
    float radii() const { return trs ? _radii * trs->s().x : _radii; }

    glm::vec3 _center;
    float _radii;
};

struct AABB : public Geometry {
    AABB() = default;
    AABB &construct(const glm::vec3 &min, const glm::vec3 &max);
    AABB &construct(const glm::vec3 *points, size_t length, TRS *trs) override;

    virtual ~AABB() noexcept {}

    std::pair<glm::vec3, glm::vec3> minMax() const;
    glm::vec3 extents() const;
    glm::vec3 _center;
    glm::vec3 _extents;
    Type type() override { return Type::AABB; }
    bool isTrivialIntersect(Geometry &other) override { return other._isTrivialIntersect(*this); }
    bool _isTrivialIntersect(struct Plane &) override { return true; }
    bool _isTrivialIntersect(struct Sphere &) override { return true; }
    bool _isTrivialIntersect(struct AABB &) override { return true; }
    // bool _isTrivialIntersect(struct OBB &other) override{ return true; }
    bool intersects(geom::Geometry &other) override { return other._intersects(*this); }
    bool _intersects(struct Plane &other) override;
    bool _intersects(struct Sphere &other) override;
    bool _intersects(struct AABB &other) override;
    // bool _intersects(struct OBB &other) override;
    bool rayIntersect(const glm::vec3 &p, const glm::vec3 &d, float &tmin, float &tmax) override;
    std::pair<glm::vec3, float> separation(Geometry &other) override {
        return other._separation(*this);
    }
    std::pair<glm::vec3, float> _separation(struct Plane &other) override;
    std::pair<glm::vec3, float> _separation(struct Sphere &other) override;
    std::pair<glm::vec3, float> _separation(struct AABB &other) override;
    // std::pair<glm::vec3, float> _separation(struct OBB &other) override;
    glm::vec3 origin() override;
    glm::vec3 supportPoint(const glm::vec3 &D) override;
};

struct OBB : public Geometry {
    OBB() = default;
    OBB &construct(const glm::vec3 &center, const glm::vec3 &extents, TRS *trs);
    OBB &construct(const glm::vec3 *points, size_t length, TRS *trs) override;

    virtual ~OBB() noexcept {}

    glm::vec3 _center;
    glm::vec3 _extents;
    glm::vec3 extents();
    Type type() override { return Type::OBB; }
    bool isTrivialIntersect(Geometry &other) override { return other._isTrivialIntersect(*this); }
    bool _isTrivialIntersect(struct Plane &) override { return true; }
    bool _isTrivialIntersect(struct Sphere &) override { return true; }
    // bool _isTrivialIntersect(struct AABB &other) override {return true; }
    bool _isTrivialIntersect(struct OBB &) override { return true; }
    bool intersects(geom::Geometry &other) override { return other._intersects(*this); }
    bool _intersects(struct Plane &other) override;
    bool _intersects(struct Sphere &other) override;
    // bool _intersects(struct AABB &other) override;
    bool _intersects(struct OBB &other) override;
    bool rayIntersect(const glm::vec3 &, const glm::vec3 &, float &, float &) override {
        return false;
    }
    std::pair<glm::vec3, float> separation(Geometry &other) override {
        return other._separation(*this);
    }
    std::pair<glm::vec3, float> _separation(struct Plane &other) override;
    std::pair<glm::vec3, float> _separation(struct Sphere &other) override;
    // std::pair<glm::vec3, float> _separation(struct AABB &other) override;
    std::pair<glm::vec3, float> _separation(struct OBB &other) override;
    glm::vec3 origin() override;
    glm::vec3 supportPoint(const glm::vec3 &D) override;
};

Plane *createPlane();
Sphere *createSphere();
AABB *createAABB();
OBB *createOBB();

void free(Geometry *geometry);
void freePlane(Plane *plane);
void freeSphere(Sphere *sphere);
void freeAABB(AABB *aabb);
void freeOBB(OBB *obb);

} // namespace geom

// #pragma clang diagnostic pop