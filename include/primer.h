#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <utility>

namespace primer {

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

constexpr std::pair<glm::vec3, glm::vec3> minMaxOf(glm::vec3 *points, size_t length);

} // namespace primer

constexpr std::pair<glm::vec3, glm::vec3> primer::minMaxOf(glm::vec3 *points, size_t length) {
    glm::vec3 min{points[0]};
    glm::vec3 max{points[0]};
    for (size_t i{1}; i < length; ++i) {
        min.x = std::min(min.x, points[i].x);
        min.y = std::min(min.y, points[i].y);
        min.z = std::min(min.z, points[i].z);

        max.x = std::max(max.x, points[i].x);
        max.y = std::max(max.y, points[i].y);
        max.z = std::max(max.z, points[i].z);
    }
    return {min, max};
}