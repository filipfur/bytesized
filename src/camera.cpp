#include "camera.h"

#include "primer.h"

Camera::Camera(const glm::vec3 &center_, float yaw_, float pitch_, float distance_)
    : currentView{center_, yaw_, pitch_, distance_}, targetView{center_, yaw_, pitch_, distance_},
      _view{}, _dirtyView{true} {}

void Camera::set(const CameraView &cameraView) {
    currentView = cameraView;
    targetView = cameraView;
}

void Camera::update(float dt) {
    float minAngle = primer::angleDistance(currentView.yaw, targetView.yaw);
    float absMinAngle = std::abs(minAngle);
    if (absMinAngle > 0.1f) {
        currentView.yaw += (minAngle / absMinAngle) * rotationalSpeed.x * dt;
    }
    // printf("current.yaw=%f target.yaw=%f\n", currentView.yaw, targetView.yaw);
    currentView.yaw = glm::mix(currentView.yaw, targetView.yaw, rotationalSpeed.x * dt);
    currentView.pitch = glm::mix(currentView.pitch, targetView.pitch, rotationalSpeed.y * dt);
    currentView.center = glm::mix(currentView.center, targetView.center, moveSpeed * dt);
    currentView.distance = glm::mix(currentView.distance, targetView.distance, zoomSpeed * dt);
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

void Camera::yawFromDirection(const glm::vec3 &from, const glm::vec3 &to) {
    return yawFromDirection(glm::normalize(to - from));
}
void Camera::yawFromDirection(const glm::vec3 &d) {
    currentView.yaw = primer::normalizedAngle(currentView.yaw);
    targetView.yaw = primer::normalizedAngle(targetView.yaw);
    targetView.yaw = glm::degrees(std::atan2f(-d.x, -d.z));
}