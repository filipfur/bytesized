#pragma once

#include <glm/glm.hpp>

// Note: Don't ever try to use quaternions cause they are a huge bait and they suck
struct CameraView {
    glm::vec3 center;
    float yaw;
    float pitch;
    float distance;
};

struct Camera {
    Camera(const glm::vec3 &center_, float yaw_, float pitch_, float distance_);

    void set(const CameraView &cameraView);

    void update(float dt);
    const glm::mat4 &view();
    const glm::vec3 &orientation();

    CameraView currentView;
    CameraView targetView;

  private:
    glm::vec3 _orientation{1.0f, 0.0f, 0.0f};
    glm::mat4 _view;
    bool _dirtyView;
};