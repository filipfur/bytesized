#include "editor.h"

#include "primer.h"
#include <cstdio>

static std::pair<glm::vec3, glm::vec3> _rightForward(float yaw) {
    glm::vec3 v{glm::cos(yaw), 0.0f, glm::sin(yaw)};
    glm::vec3 right = glm::cross(v, primer::UP);
    glm::vec3 forward = glm::cross(primer::UP, right);
    return {right, forward};
}

HistoricEvent &Editor::emplaceHistoricEvent(HistoricEvent::Type type) {
    auto &hist = historicEvents.emplace_back();
    futureEvents.clear();
    hist.type = type;
    return hist;
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
        _rotation = historicEvents.back().rotation;
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

bool Editor::mouseDown(int /*button*/, int /*x*/, int /*y*/) {
    if (_editMode != EditMode::NONE) {
        _editMode = EditMode::NONE;
        _editAxis = EditAxis::NONE;
        return true;
    }
    return false;
}
bool Editor::mouseUp(int /*button*/, int /*x*/, int /*y*/) { return false; }

bool Editor::mouseMoved(float /*x*/, float /*y*/, float xrel, float yrel) {
    if (_selectedNode == nullptr) {
        return false;
    }
    auto [right, forward] = _rightForward(_currentView.yaw);
    switch (_editMode) {
    case EditMode::TRANSLATE:
        if (_editAxis != EditAxis::NONE) {
            const glm::vec3 &axis = primer::AXES[static_cast<size_t>(_editAxis) - 1];
            _translation +=
                axis *
                glm::dot(axis, (right * -xrel +
                                (_editAxis == EditAxis::Y ? -primer::UP : forward) * yrel)) *
                0.05f;
        } else {
            _translation += (right * -xrel + forward * yrel) * 0.05f;
        }
        _selectedNode->translation = applyStep(_translation, _tstep);
        break;
    case EditMode::ROTATE:
        switch (_editAxis) {
        case EditAxis::X:
            _rotation *= glm::angleAxis(
                glm::radians(glm::dot(primer::AXIS_Z, right * -xrel + forward * yrel)),
                primer::AXIS_X);
            break;
        case EditAxis::Y:
            _rotation *= glm::angleAxis(glm::radians(xrel), primer::AXIS_Y);
            break;
        case EditAxis::Z:
            _rotation *= glm::angleAxis(
                glm::radians(glm::dot(-primer::AXIS_X, right * -xrel + forward * yrel)),
                primer::AXIS_Z);
            break;
        case EditAxis::NONE:
            _rotation *= glm::angleAxis(glm::radians(xrel), primer::UP);
            //_rotation *= glm::angleAxis(glm::radians(yrel), forward);
            break;
        }
        _selectedNode->rotation = _rotation;
        break;
        return false;
    case EditMode::SCALE:
        switch (_editAxis) {
        case EditAxis::X:
        case EditAxis::Y:
        case EditAxis::Z:
            _selectedNode->scale.data()[static_cast<size_t>(_editAxis) - 1] *=
                ((xrel - yrel) > 0 ? 1.0f / 0.9f : 0.9f);
            _selectedNode->invalidate();
            break;
        case EditAxis::NONE:
            _selectedNode->scale *= ((xrel - yrel) > 0 ? glm::vec3{1.0f / 0.9f} : glm::vec3{0.9f});
            break;
        }
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

gpu::Node *Editor::addNode(gpu::Node *node) {
    node->translation = _selectedNode ? _selectedNode->translation.data() : _targetView.center;
    node->rotation = _selectedNode ? _selectedNode->rotation.data()
                                   : glm::angleAxis(_currentView.yaw, primer::UP);
    node->scale = _selectedNode ? _selectedNode->scale.data() : glm::vec3{1.0f, 1.0f, 1.0f};
    auto &ev = emplaceHistoricEvent(HistoricEvent::ADD);
    ev.nodes.emplace_back(node);
    _iEditor->nodeAdded(node);
    return node;
}
gpu::Node *Editor::removeNode(gpu::Node *node) {
    auto &ev = emplaceHistoricEvent(HistoricEvent::REMOVE);
    ev.nodes.emplace_back(node);
    _iEditor->nodeRemoved(node);
    return node;
}

gpu::Node *Editor::selectNode(gpu::Node *node) {
    _selectedNode = node;
    _iEditor->nodeSelected(_selectedNode);
    return _selectedNode;
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
            _rotation = _selectedNode->rotation.data();
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
            break;
        }
        return true;
    default:
        break;
    }
    return false;
}

bool Editor::keyUp(int /*key*/, int /*mods*/) { return false; }

void Editor::update(float dt) {
    _currentView.yaw = glm::mix(_currentView.yaw, _targetView.yaw, 4.0f * dt);
    _currentView.pitch = glm::mix(_currentView.pitch, _targetView.pitch, 4.0f * dt);
    _currentView.center = glm::mix(_currentView.center, _targetView.center, 4.0f * dt);
    _currentView.distance = glm::mix(_currentView.distance, _targetView.distance, 4.0f * dt);
    _dirtyView = true;
}

const glm::mat4 &Editor::view() {
    if (_dirtyView) {
        glm::vec3 orientation{glm::cos(_currentView.yaw) * glm::cos(_currentView.pitch),
                              glm::sin(_currentView.pitch),
                              glm::sin(_currentView.yaw) * glm::cos(_currentView.pitch)};
        _view = glm::lookAt(_currentView.center + orientation * _currentView.distance,
                            _currentView.center, primer::UP);
        _dirtyView = false;
    }
    return _view;
}

bool Editor::translateSelected(const glm::vec3 &translation) {
    if (_selectedNode) {
        auto &ev = emplaceHistoricEvent(HistoricEvent::TRANSLATE);
        ev.nodes.emplace_back(_selectedNode);
        ev.translation = _selectedNode->translation.data();
        _selectedNode->translation = translation;
        _iEditor->nodeTransformed(_selectedNode);
        return true;
    }
    return false;
}

bool Editor::rotateSelected(const glm::quat &rotation) {
    if (_selectedNode) {
        auto &ev = emplaceHistoricEvent(HistoricEvent::ROTATE);
        ev.nodes.emplace_back(_selectedNode);
        ev.rotation = _selectedNode->rotation.data();
        _selectedNode->rotation = rotation;
        _iEditor->nodeTransformed(_selectedNode);
        return true;
    }
    return false;
}

bool Editor::scaleSelected(const glm::vec3 &scale) {
    if (_selectedNode) {
        auto &ev = emplaceHistoricEvent(HistoricEvent::SCALE);
        ev.nodes.emplace_back(_selectedNode);
        ev.scale = _selectedNode->scale.data();
        _selectedNode->scale = scale;
        _iEditor->nodeTransformed(_selectedNode);
        return true;
    }
    return false;
}

bool Editor::hasHistory() { return !historicEvents.empty(); }

void Editor::clearHistory() {
    historicEvents.clear();
    futureEvents.clear();
}

void Editor::enable() {
    _enabled = true;
    window::enableMouseListener(this);
    window::enableMouseMotionListener(this);
    window::enableMouseWheelListener(this);
    window::enableKeyListener(this);
}
void Editor::disable() {
    _enabled = false;
    window::disableMouseListener(this);
    window::disableMouseMotionListener(this);
    window::disableMouseWheelListener(this);
    window::disableKeyListener(this);
}

void Editor::setCurrentView(const CameraView &cameraView) { _currentView = cameraView; }
void Editor::setTargetView(const CameraView &cameraView) { _targetView = cameraView; }