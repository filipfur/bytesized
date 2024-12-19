#include "editor.h"

#define GLM_ENABLE_EXPERIMENTAL

#include <cstdio>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

static std::pair<glm::vec3, glm::vec3> _rightForward(float yaw) {
    glm::vec3 v{cosf(yaw), 0.0f, sinf(yaw)};
    glm::vec3 right = glm::cross(v, UP);
    glm::vec3 forward = glm::cross(UP, right);
    return {right, forward};
}

HistoricEvent &Editor::emplaceHistoricEvent(HistoricEvent::Type type) {
    auto &hist = historicEvents.emplace_back();
    futureEvents.clear();
    hist.type = type;
    return hist;
}

inline static glm::vec3 eulerDegrees(const glm::quat &q) {
    /*
    float y, p, r;
    glm::extractEulerAngleYXZ(glm::mat4_cast(q), y, p, r);
    return glm::vec3{p, y, r};
    */
    return glm::eulerAngles(q) / glm::pi<float>() * 180.0f;
}

template <typename T> static T applyStep(T x, float step) {
    return step < FLT_EPSILON ? x : glm::round(x / step) * step;
}

void Editor::_resetHistoric() {
    switch (historicEvents.back().type) {
    case HistoricEvent::TRANSLATE:
        _selectedNode->translation = historicEvents.back().translation;
        _translation = historicEvents.back().translation;
        break;
    case HistoricEvent::ROTATE:
        _selectedNode->rotation = historicEvents.back().rotation;
        _rotation = eulerDegrees(historicEvents.back().rotation);
        break;
    case HistoricEvent::SCALE:
        _selectedNode->scale = historicEvents.back().scale;
        break;
    default:
        break;
    }
}

bool Editor::undo_redo(std::list<HistoricEvent> &eventsListA,
                       std::list<HistoricEvent> &eventsListB) {
    if (eventsListA.empty()) {
        return false;
    }
    auto &ev = eventsListA.back();
    auto &ev2 = eventsListB.emplace_back();
    ev2.type = ev.type;
    ev2.nodes.resize(ev.nodes.size());
    std::copy(ev.nodes.begin(), ev.nodes.end(), ev2.nodes.begin());
    for (gpu::Node *node : ev.nodes) {
        switch (ev.type) {
        case HistoricEvent::TRANSLATE:
            ev2.translation = node->translation.data();
            node->translation = ev.translation;
            break;
        case HistoricEvent::ROTATE:
            ev2.rotation = node->rotation.data();
            node->rotation = ev.rotation;
            break;
        case HistoricEvent::SCALE:
            ev2.scale = node->scale.data();
            node->scale = ev.scale;
            break;
        case HistoricEvent::ADD:
            ev2.type = HistoricEvent::REMOVE;
            _iEditor->nodeRemoved(node);
            break;
        case HistoricEvent::REMOVE:
            ev2.type = HistoricEvent::ADD;
            _iEditor->nodeAdded(node);
            break;
        default:
            return false;
        }
    }
    eventsListA.pop_back();
    return true;
}

bool Editor::mouseDown(int button, int x, int y) {
    if (_editMode != EditMode::NONE) {
        _editMode = EditMode::NONE;
        _editAxis = EditAxis::NONE;
        return true;
    }
    return false;
}
bool Editor::mouseUp(int button, int x, int y) { return false; }

bool Editor::mouseMoved(float x, float y, float xrel, float yrel) {
    if (_selectedNode == nullptr) {
        return false;
    }
    auto [right, forward] = _rightForward(_currentView.yaw);
    switch (_editMode) {
    case EditMode::TRANSLATE:
        if (_editAxis != EditAxis::NONE) {
            const glm::vec3 &axis = AXES[static_cast<size_t>(_editAxis) - 1];
            _translation += axis *
                            glm::dot(axis, (right * -xrel +
                                            (_editAxis == EditAxis::Y ? -UP : forward) * yrel)) *
                            0.05f;
        } else {
            _translation += (right * -xrel + forward * yrel) * 0.05f;
        }
        _selectedNode->translation = applyStep(_translation, _tstep);
        break;
    case EditMode::ROTATE:
        if ((xrel * xrel) > FLT_EPSILON) {
            const EditAxis ax = _editAxis == EditAxis::NONE ? EditAxis::Y : _editAxis;
            const glm::vec3 &axis = AXES[static_cast<size_t>(ax) - 1];
            /*const glm::vec3 rotv = right * -yrel + (ax == EditAxis::Y ? -UP : forward) * xrel;
            if (glm::length(rotv) > 4.0f) {
                break;
            }*/
            _rotation += axis * xrel;
            // axis * glm::dot(axis, (right * -yrel + (ax == EditAxis::Y ? -UP : forward) * xrel));
            _rotation.x = _rotation.x > +180.0f ? _rotation.x - 360.0f : _rotation.x;
            _rotation.x = _rotation.x < -180.0f ? _rotation.x + 360.0f : _rotation.x;
            _rotation.y = _rotation.y > +180.0f ? _rotation.y - 360.0f : _rotation.y;
            _rotation.y = _rotation.y < -180.0f ? _rotation.y + 360.0f : _rotation.y;
            _rotation.z = _rotation.z > +180.0f ? _rotation.z - 360.0f : _rotation.z;
            _rotation.z = _rotation.z < -180.0f ? _rotation.z + 360.0f : _rotation.z;

            const glm::vec3 rot = applyStep(_rotation, _rstep);
            _selectedNode->rotation = glm::angleAxis(glm::radians(rot.x), AXIS_X) *
                                      glm::angleAxis(glm::radians(rot.y), AXIS_Y) *
                                      glm::angleAxis(glm::radians(rot.z), AXIS_Z);
            break;
        }
        return false;
    case EditMode::SCALE:
        _selectedNode->scale *= ((xrel - yrel) > 0 ? glm::vec3{1.0f / 0.9f} : glm::vec3{0.9f});
        break;
    default:
        return false;
    }
    _iEditor->nodeTransformed(_selectedNode);
    return true;
}

bool Editor::mouseScrolled(float x, float y) {
    SDL_Keymod mod = SDL_GetModState();

    static const float _urad = glm::radians(1.0f);
    x *= _urad;
    y *= -_urad;

    _dirtyView = true;

    if (mod & KMOD_SHIFT) {
        auto [right, forward] = _rightForward(_currentView.yaw);
        _targetView.center += (right * -x + forward * y) * 10.0f;
        _targetView.center.x = glm::round(_targetView.center.x * 10.0f) * 0.1f;
        _targetView.center.z = glm::round(_targetView.center.z * 10.0f) * 0.1f;
    } else if (mod & KMOD_LGUI) {
        _targetView.center.y += y * 5.0f;
        _targetView.center.y = glm::round(_targetView.center.y * 10.0f) * 0.1f;
    } else {
        _targetView.yaw -= x * 5.0f;
        _targetView.pitch -= y * 5.0f;
    }

    return true;
}

void Editor::addNode(gpu::Node *node) {
    node->translation = _selectedNode ? _selectedNode->translation.data() : _targetView.center;
    node->rotation =
        _selectedNode ? _selectedNode->rotation.data() : glm::angleAxis(_currentView.yaw, UP);
    node->scale = _selectedNode ? _selectedNode->scale.data() : glm::vec3{1.0f, 1.0f, 1.0f};
    auto &ev = emplaceHistoricEvent(HistoricEvent::ADD);
    ev.nodes.emplace_back(node);
    _iEditor->nodeAdded(node);
}
void Editor::removeNode(gpu::Node *node) {
    auto &ev = emplaceHistoricEvent(HistoricEvent::REMOVE);
    ev.nodes.emplace_back(node);
    _iEditor->nodeRemoved(node);
}

void Editor::selectNode(gpu::Node *node) {
    _selectedNode = node;
    _iEditor->nodeSelected(_selectedNode);
}

gpu::Node *Editor::startTranslation() {
    if (_selectedNode) {
        _editMode = EditMode::TRANSLATE;
        _translation = _selectedNode->translation.data();
        auto &ev = emplaceHistoricEvent(HistoricEvent::TRANSLATE);
        ev.nodes.emplace_back(_selectedNode);
        ev.translation = _selectedNode->translation.data();
        return _selectedNode;
    }
    return nullptr;
}

bool Editor::keyDown(int key, int mods) {
    if (SDL_IsTextInputActive()) {
        return false;
    }
    switch (key) {
    case SDLK_TAB:
        selectNode(_iEditor->cycleNode(_selectedNode));
        // fall-through
    case SDLK_SPACE:
        if (_selectedNode) {
            _targetView.center = _selectedNode->translation.data();
        }
        return true;
    case SDLK_MINUS:
        if (_targetView.distance <= 1.0f) {
            _targetView.distance /= 0.9f;
        } else {
            _targetView.distance = glm::round(_targetView.distance + 1.0f);
        }
        return true;
    case SDLK_PLUS:
        if (_targetView.distance <= 1.0f) {
            _targetView.distance *= 0.9f;
        } else {
            _targetView.distance = glm::round(_targetView.distance - 1.0f);
        }
        return true;
    case SDLK_g:
    case SDLK_t:
        startTranslation();
        return true;
    case SDLK_r:
        if (mods & KMOD_CTRL) {
            undo_redo(futureEvents, historicEvents);
        } else if (_selectedNode) {
            _editMode = EditMode::ROTATE;
            _rotation = eulerDegrees(_selectedNode->rotation.data());
            auto &ev = emplaceHistoricEvent(HistoricEvent::ROTATE);
            ev.nodes.emplace_back(_selectedNode);
            ev.rotation = _selectedNode->rotation.data();
        }
        return true;
    case SDLK_s:
        if (_selectedNode) {
            _editMode = EditMode::SCALE;
            auto &ev = emplaceHistoricEvent(HistoricEvent::SCALE);
            ev.nodes.emplace_back(_selectedNode);
            ev.scale = _selectedNode->scale.data();
        }
        return true;
    case SDLK_u:
        undo_redo(historicEvents, futureEvents);
        return true;
    case SDLK_c:
        if ((mods & KMOD_LGUI) == 0 || _editMode != EditMode::NONE) {
            break;
        }
        // fall-through
    case SDLK_y:
        if (_editMode != EditMode::NONE) {
            _editAxis = _editAxis != EditAxis::Y ? EditAxis::Y : EditAxis::NONE;
            _resetHistoric();
            _iEditor->nodeTransformed(_selectedNode);
            return true;
        }
        if (_selectedNode) { // copy
            _carbonNode = _selectedNode;
            return true;
        }
        break;
    case SDLK_v:
        if ((mods & KMOD_LGUI) == 0) {
            break;
        }
        // fall-through
    case SDLK_p: // paste
        if (_carbonNode) {
            _iEditor->nodeCopied(_carbonNode);
            return true;
        }
        break;
    case SDLK_x:
        if (_editMode != EditMode::NONE) {
            _editAxis = _editAxis != EditAxis::X ? EditAxis::X : EditAxis::NONE;
            _resetHistoric();
            _iEditor->nodeTransformed(_selectedNode);
            return true;
        }
        removeNode(_selectedNode);
        return true;
    case SDLK_z:
        if (_editMode != EditMode::NONE) {
            _editAxis = _editAxis != EditAxis::Z ? EditAxis::Z : EditAxis::NONE;
            _resetHistoric();
            _iEditor->nodeTransformed(_selectedNode);
            return true;
        }
        if ((mods & (KMOD_LGUI | KMOD_LSHIFT)) == (KMOD_LGUI | KMOD_LSHIFT)) {
            undo_redo(futureEvents, historicEvents);
        } else if (mods & KMOD_LGUI) {
            undo_redo(historicEvents, futureEvents);
        }
        return true;
    case SDLK_ESCAPE:
        if (_editMode == EditMode::TRANSLATE) {
            _editMode = EditMode::NONE;
            _editAxis = EditAxis::NONE;
            _selectedNode->translation = historicEvents.back().translation;
            historicEvents.pop_back();
            _iEditor->nodeTransformed(_selectedNode);
        } else if (_editMode == EditMode::ROTATE) {
            _editMode = EditMode::NONE;
            _editAxis = EditAxis::NONE;
            _selectedNode->rotation = historicEvents.back().rotation;
            historicEvents.pop_back();
            _iEditor->nodeTransformed(_selectedNode);
        } else if (_editMode == EditMode::SCALE) {
            _editMode = EditMode::NONE;
            _editAxis = EditAxis::NONE;
            _selectedNode->scale = historicEvents.back().scale;
            historicEvents.pop_back();
            _iEditor->nodeTransformed(_selectedNode);
        } else if (_selectedNode) {
            _cameraMode = CameraMode::PLANAR;
            _selectedNode = nullptr;
            _iEditor->nodeSelected(nullptr);
        } else {
            exit(0);
        }
        return true;
    }
    return false;
}

bool Editor::keyUp(int key, int mods) { return false; }

void Editor::update(float dt) {
    _currentView.yaw = glm::mix(_currentView.yaw, _targetView.yaw, 4.0f * dt);
    _currentView.pitch = glm::mix(_currentView.pitch, _targetView.pitch, 4.0f * dt);
    _currentView.center = glm::mix(_currentView.center, _targetView.center, 4.0f * dt);
    _currentView.distance = glm::mix(_currentView.distance, _targetView.distance, 4.0f * dt);
    _dirtyView = true;
}

const glm::mat4 &Editor::view() {
    if (_dirtyView) {
        glm::vec3 orientation{cosf(_currentView.yaw) * cosf(_currentView.pitch),
                              sinf(_currentView.pitch),
                              sinf(_currentView.yaw) * cosf(_currentView.pitch)};
        _view = glm::lookAt(_currentView.center + orientation * _currentView.distance,
                            _currentView.center, UP);
        _dirtyView = false;
    }
    return _view;
}

void Editor::translateSelected(const glm::vec3 &translation) {
    if (_selectedNode) {
        auto &ev = emplaceHistoricEvent(HistoricEvent::TRANSLATE);
        ev.nodes.emplace_back(_selectedNode);
        ev.translation = _selectedNode->translation.data();
        _selectedNode->translation = translation;
    }
}

void Editor::rotateSelected(const glm::quat &rotation) {
    if (_selectedNode) {
        auto &ev = emplaceHistoricEvent(HistoricEvent::ROTATE);
        ev.nodes.emplace_back(_selectedNode);
        ev.rotation = _selectedNode->rotation.data();
        _selectedNode->rotation = rotation;
    }
}

void Editor::scaleSelected(const glm::vec3 &scale) {
    if (_selectedNode) {
        auto &ev = emplaceHistoricEvent(HistoricEvent::SCALE);
        ev.nodes.emplace_back(_selectedNode);
        ev.scale = _selectedNode->scale.data();
        _selectedNode->scale = scale;
    }
}

void Editor::setCurrentView(const CameraView &cameraView) { _currentView = cameraView; }
void Editor::setTargetView(const CameraView &cameraView) { _targetView = cameraView; }