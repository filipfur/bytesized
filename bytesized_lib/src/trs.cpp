#include "trs.h"

#include <glm/glm.hpp>

static glm::quat fromEuler(const glm::vec3 &rotation) {
    return glm::angleAxis(glm::radians(rotation.y), glm::vec3{0.0f, 1.0f, 0.0f}) *
           glm::angleAxis(glm::radians(rotation.x), glm::vec3{1.0f, 0.0f, 0.0f}) *
           glm::angleAxis(glm::radians(rotation.z), glm::vec3{0.0f, 0.0f, 1.0f});
}

glm::mat4 &TRS::model() {
    if (!_valid) {
        if (_invalidEuler) {
            rotation = fromEuler(euler.data());
            _invalidEuler = false;
        }
        glm::mat4 local = glm::scale(glm::translate(glm::mat4(1.0f), translation.data()) *
                                         glm::mat4(glm::mat3_cast(rotation.data())),
                                     scale.data());
        if (_parent) {
            _model = _parent->model() * local;
        } else {
            _model = local;
        }
        _valid = true;
    }
    return _model;
}

glm::vec3 TRS::position() {
    auto &m = model();
    return {m[3][0], m[3][1], m[3][2]};
}

TRS *TRS::parent() { return _parent; }

void TRS::setParent(TRS *parent) { _parent = parent; }