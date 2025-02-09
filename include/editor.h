#pragma once

#include "window.h"

#include "camera.h"
#include "gpu.h"
#include <glm/glm.hpp>
#include <list>

struct HistoricEvent {
    enum Type { NONE, TRANSLATE, ROTATE, SCALE, ADD, REMOVE } type;
    std::vector<gpu::Node *> nodes;
    union {
        glm::vec3 translation;
        glm::vec3 euler;
        glm::vec3 scale;
    };
};

#ifndef EMSCRIPTEN
static_assert(sizeof(HistoricEvent) == 48);
static_assert(sizeof(gpu::Node *) == 8);
static_assert(sizeof(std::vector<gpu::Node *>) == 24);
static_assert(sizeof(std::list<gpu::Node *>) == 24);
#endif

struct IEditor {
    virtual void nodeSelected(gpu::Node *node) = 0;
    virtual void nodeAdded(gpu::Node *node) = 0;
    virtual void nodeRemoved(gpu::Node *node) = 0;
    virtual void nodeCopied(gpu::Node *node) = 0;
    virtual void nodeTransformed(gpu::Node *node) = 0;
    virtual gpu::Node *cycleNode(gpu::Node *node) = 0;
};

struct Editor : public window::IMouseListener,
                public window::IMouseMotionListener,
                public window::IMouseWheelListener,
                public window::IKeyListener {

    Editor(Camera &camera) : _camera{camera} {
        window::registerMouseListener(this);
        window::registerMouseMotionListener(this);
        window::registerMouseWheelListener(this);
        window::registerKeyListener(this);
    }

    bool mouseDown(int button, int x, int y) override;
    bool mouseUp(int button, int x, int y) override;
    bool mouseMoved(float x, float y, float xrel, float yrel) override;
    bool mouseScrolled(float x, float y) override;
    bool keyDown(int key, int mods) override;
    bool keyUp(int key, int mods) override;
    gpu::Node *addNode(gpu::Node *node);
    gpu::Node *removeNode(gpu::Node *node);
    gpu::Node *selectNode(gpu::Node *node);

    void registerIEditor(IEditor *iEditor) { _iEditor = iEditor; }

    enum class CameraMode { PLANAR };

    enum class EditAxis { NONE, X, Y, Z };
    enum class EditMode { NONE, TRANSLATE, ROTATE, SCALE };

    // const CameraView &currentView() { return _currentView; }
    // const CameraView &targetView() { return _targetView; }

    gpu::Node *selectedNode() { return _selectedNode; }
    gpu::Node *startTranslation();
    bool translateSelected(const glm::vec3 &translation);
    bool rotateSelected(const glm::vec3 &rotation);
    bool scaleSelected(const glm::vec3 &scale);

    // void setCurrentView(const CameraView &cameraView);
    // void setTargetView(const CameraView &cameraView);

    void setTStep(float tstep) { _tstep = tstep; }
    void setRStep(float rstep) { _rstep = rstep; }

    bool hasHistory();
    void clearHistory();

    void disable(bool disabled);

  private:
    HistoricEvent &emplaceHistoricEvent(HistoricEvent::Type type);
    bool undo_redo(std::list<HistoricEvent> &eventsListA, std::list<HistoricEvent> &eventsListB);
    void _resetHistoric();

    CameraMode _cameraMode{CameraMode::PLANAR};
    EditMode _editMode{EditMode::NONE};
    EditAxis _editAxis{EditAxis::NONE};
    Camera &_camera;
    float _tstep{0.0f};
    float _rstep{0.0f};
    bool _dirtyView{true};
    glm::vec3 _translation;
    glm::vec3 _euler;
    // glm::vec3 _scale;
    std::list<HistoricEvent> historicEvents;
    std::list<HistoricEvent> futureEvents;
    glm::mat4 _view{1.0f};
    IEditor *_iEditor{nullptr};
    gpu::Node *_selectedNode{nullptr};
    gpu::Node *_carbonNode{nullptr};
    bool _disabled{true};
};