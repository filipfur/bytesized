#pragma once

#include "gpu.h"
#include "library.h"
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

namespace assets {

struct Texture {
    uint32_t *id;
};

struct Collection {
    Collection();
    Collection(const library::Collection &collection);
    gpu::Animation *animationByName(const char *name);
    const std::string &name() const;

    gpu::Scene *scene;
    std::vector<Texture> texture;
    std::vector<gpu::Animation *> animations;
};
} // namespace assets
