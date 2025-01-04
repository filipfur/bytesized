#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include "trs.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/string_cast.hpp>
#include <utility>

namespace primer {

const static glm::vec3 UP{0.0f, 1.0f, 0.0f};
const static glm::vec3 AXIS_X{1.0f, 0.0f, 0.0f};
const static glm::vec3 AXIS_Y{0.0f, 1.0f, 0.0f};
const static glm::vec3 AXIS_Z{0.0f, 0.0f, 1.0f};
const static glm::vec3 AXES[] = {AXIS_X, AXIS_Y, AXIS_Z};

inline static glm::vec3 eulerDegrees(const glm::quat &q) {
    /*
    float y, p, r;
    glm::extractEulerAngleYXZ(glm::mat4_cast(q), y, p, r);
    return glm::vec3{p, y, r};
    */
    return glm::eulerAngles(q) / glm::pi<float>() * 180.0f;
}

inline static glm::vec3 eulerDegrees2(const glm::quat &q) {
    /*
    float y, p, r;
    glm::extractEulerAngleYXZ(glm::mat4_cast(q), y, p, r);
    return glm::vec3{p, y, r};
    */
    return glm::eulerAngles(q) / glm::pi<float>() * 180.0f;
}

inline static bool pointInTriangle(const glm::vec3 &p, const glm::vec3 &a, const glm::vec3 &b,
                                   const glm::vec3 &c) {
    const glm::vec3 A = a - p;
    const glm::vec3 B = b - p;
    const glm::vec3 C = c - p;
    const glm::vec3 u = glm::cross(A, B);
    const glm::vec3 v = glm::cross(B, C);
    const glm::vec3 w = glm::cross(C, A);
    float d1 = glm::dot(u, v);
    float d2 = glm::dot(u, w);
    if (d1 < 0) {
        return false;
    }
    if (d2 < 0) {
        return false;
    }
    return true;
}

inline static glm::vec3 barycentric(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    glm::vec3 v0 = b - a;
    glm::vec3 v1 = c - a;
    glm::vec3 v2 = p - a;

    float d00 = glm::dot(v0, v0);
    float d01 = glm::dot(v0, v1);
    float d11 = glm::dot(v1, v1);
    float d20 = glm::dot(v2, v0);
    float d21 = glm::dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;

    glm::vec3 barycentric;
    barycentric[0] = (d11 * d20 - d01 * d21) / denom;
    barycentric[1] = (d00 * d21 - d01 * d20) / denom;
    barycentric[2] = 1.0f - barycentric[0] - barycentric[1];
    return barycentric;
}

inline static std::pair<int, float> indexAlongDirection(const std::vector<glm::vec3> &s,
                                                        const glm::vec3 &D) {
    int index{-1};
    float maxValue{-FLT_MAX}; // glm::dot(s[0], D)};

    for (size_t i{0}; i < s.size(); ++i) {
        float value = glm::dot(s[i], D);
        if (value > maxValue) {
            index = static_cast<int>(i);
            maxValue = value;
        }
    }
    return {index, maxValue};
}

inline static glm::quat quaternionFromDirection(const glm::vec3 &direction) {
    glm::vec3 n = glm::normalize(direction);
    float angle;
    glm::vec3 axis;

    if (n.y >= 1.0f) {
        angle = 0.0f;
        axis = glm::vec3{1.0f, 0.0f, 0.0f};
    } else if (n.y <= -1.0f) {
        angle = glm::pi<float>();
        axis = glm::vec3{1.0f, 0.0f, 0.0f};
    } else {
        axis = glm::normalize(glm::cross(UP, n));
        angle = acosf(glm::dot(UP, n));
    }

    return glm::angleAxis(angle, axis);
}

inline static glm::vec3 directionFromQuaternion(const glm::quat &quat) { return quat * UP; }

static constexpr float EPSILON = 1e-5f;
inline static bool isNear(float a, float b) { return ((a > b) ? (a - b) : (b - a)) < EPSILON; }
inline static bool isNear(const glm::vec3 &a, const glm::vec3 &b) {
    return isNear(a.x, b.x) && isNear(a.y, b.y) && isNear(a.z, b.z);
}

constexpr glm::vec3 transformPoint(const glm::mat4 &m, const glm::vec3 &point);
constexpr std::pair<glm::vec3, glm::vec3> minMaxOf(const glm::vec3 *points, size_t length,
                                                   TRS *trs = nullptr);
constexpr std::pair<glm::vec3, glm::vec3> tangentsOf(const glm::vec3 &normal);

} // namespace primer

constexpr glm::vec3 primer::transformPoint(const glm::mat4 &m, const glm::vec3 &point) {
    return glm::vec3(m * glm::vec4(point, 1.0f));
}

#define print_glm(var) printf(#var "=%s\n", glm::to_string(var).c_str())

constexpr std::pair<glm::vec3, glm::vec3> primer::minMaxOf(const glm::vec3 *points, size_t length,
                                                           TRS *trs) {
    glm::vec3 p = trs ? transformPoint(trs->model(), points[0]) : points[0];
    glm::vec3 min{p};
    glm::vec3 max{p};
    for (size_t i{1}; i < length; ++i) {
        p = trs ? transformPoint(trs->model(), points[i]) : points[i];
        min.x = std::min(min.x, p.x);
        min.y = std::min(min.y, p.y);
        min.z = std::min(min.z, p.z);
        max.x = std::max(max.x, p.x);
        max.y = std::max(max.y, p.y);
        max.z = std::max(max.z, p.z);
    }
    return {min, max};
}

constexpr std::pair<glm::vec3, glm::vec3> tangentsOf(const glm::vec3 &normal) {
    const glm::vec3 tangent = glm::cross(normal, glm::vec3{normal.z, normal.x, normal.y});
    const glm::vec3 bitangent = glm::cross(tangent, normal);
    return {tangent, bitangent};
}