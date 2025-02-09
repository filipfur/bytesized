#pragma once

#include "component.h"

struct Character {
    gpu::Node *node;
    Actor *actor;
    Controller *ctrl;
};