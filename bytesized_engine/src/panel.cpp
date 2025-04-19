#include "panel.h"

static constexpr size_t MAX_PANELS = 10;
Panel _panels[MAX_PANELS];
static constexpr CameraView defaultCameraView{{0.0f, 0.0f, 0.0f}, 0.0f, -15.0f, 10.0f};

Panel *Panel_assign(Panel::Type type, void *data) {
    for (auto &panel : _panels) {
        if (panel.data == data) {
            assert(panel.type == panel.type);
            return &panel;
        }
    }
    for (auto &panel : _panels) {
        if (panel.type == Panel::UNUSED) {
            panel.type = type;
            panel.data = data;
            panel.cameraView = defaultCameraView;
            return &panel;
        }
    }
    return nullptr;
}

Panel *Panel_unassign(Panel *panel) {
    panel->type = Panel::UNUSED;
    panel->data = nullptr;
    panel->cameraView = defaultCameraView;
    return panel;
}

Panel *Panel_byIndex(size_t index) {
    assert(index < MAX_PANELS);
    return _panels + index;
}

Panel *Panel_byData(void *data) {
    for (size_t i{0}; i < MAX_PANELS; ++i) {
        if (_panels[i].data == data) {
            return _panels + i;
        }
    }
    return nullptr;
}

size_t Panel_index(Panel *panel) { return (panel - _panels) / sizeof(Panel *); }