#pragma once

#include <glm/glm.hpp>

struct CameraView {
    glm::vec3 center;
    float yaw;
    float pitch;
    float distance;
};