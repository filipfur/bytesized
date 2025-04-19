#pragma once

#include "camera.h"

struct Panel {
    enum Type { UNUSED, COLLECTION, SAVE_FILE } type;
    void *data;
    CameraView cameraView;
};

Panel *Panel_assign(Panel::Type type, void *data);
Panel *Panel_unassign(Panel *panel);
Panel *Panel_byIndex(size_t index);
Panel *Panel_byData(void *data);
size_t Panel_index(Panel *panel);