#include "trs.h"

#include <glm/glm.hpp>

glm::mat4 &TRS::model() {
    if (!_valid) {

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

TRS *TRS::parent() { return _parent; }

void TRS::setParent(TRS *parent) { _parent = parent; }