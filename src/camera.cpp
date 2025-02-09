#include "camera.h"

#include "primer.h"

Camera::Camera(const glm::vec3 &center_, float yaw_, float pitch_, float distance_)
    : currentView{center_, yaw_, pitch_, distance_}, targetView{center_, yaw_, pitch_, distance_},
      _view{}, _dirtyView{true} {}

void Camera::set(const CameraView &cameraView) {
    currentView = cameraView;
    targetView = cameraView;
}

float _normalizeRadians(float rad) {
    static constexpr float pi2 = glm::pi<float>() * 2.0f;
    while (rad > glm::pi<float>()) {
        rad -= pi2;
    }
    while (rad < glm::pi<float>()) {
        rad += pi2;
    }
    return rad;
}

void Camera::update(float dt) {
    static constexpr float LERP_SPEED = 6.0f;
    currentView.yaw = glm::mix(currentView.yaw, targetView.yaw, LERP_SPEED * dt);
    currentView.pitch = glm::mix(currentView.pitch, targetView.pitch, LERP_SPEED * dt);
    currentView.center = glm::mix(currentView.center, targetView.center, LERP_SPEED * dt);
    currentView.distance = glm::mix(currentView.distance, targetView.distance, LERP_SPEED * dt);
    _dirtyView = true;
}

const glm::mat4 &Camera::view() {
    if (_dirtyView) {
        _orientation = glm::angleAxis(glm::radians(currentView.yaw), primer::UP) *
                       glm::angleAxis(glm::radians(currentView.pitch), primer::RIGHT) *
                       primer::FORWARD;
        _view = glm::lookAt(currentView.center + -_orientation * currentView.distance,
                            currentView.center, primer::UP);
        _dirtyView = false;
    }
    return _view;
}

const glm::vec3 &Camera::orientation() { return _orientation; }