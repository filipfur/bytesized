#pragma once

#include "color.h"
#include "gpu.h"

namespace skydome {
void create();

struct Sky {
    Color high;
    Color low;
    Color ambient;
    float clouds;
};
void setSky(const Sky &sky);

void render();
} // namespace skydome