#pragma once

#include "gpu.h"
#include <array>
#include <glm/glm.hpp>
#include <list>
#include <unordered_map>
#include <vector>

struct AtlasRegion {
    glm::vec2 bottomLeft;
    glm::vec2 size;
};
using AtlasRegions = std::vector<AtlasRegion>;
template <std::size_t N> struct Atlas {
    gpu::Texture *texture;
    std::array<AtlasRegions, N> regions;
};

inline static constexpr AtlasRegions Spritesheet(uint16_t cols, uint16_t rows) {
    AtlasRegions regions;
    float u = 1.0f / (float)cols;
    float v = 1.0f / (float)rows;
    for (size_t i{0}; i < cols; ++i) {
        for (size_t j{0}; j < rows; ++j) {
            regions.push_back({glm::vec2{0.0f + i * u, 0.0f + j * v}, glm::vec2{u, v} * 0.999f});
        }
    }
    return regions;
}

struct AnimationFrame {
    AnimationFrame(const AtlasRegion &r, const uint16_t d) : region{r}, duration{d} {}
    const AtlasRegion &region;
    const uint16_t duration;
};
using AnimationFrames = std::vector<const AnimationFrame>;
struct Animation {
    gpu::Texture *texture;
    AnimationFrames frames;
};
using Animations = std::vector<const Animation>;

struct Sprite {
    Sprite(glm::vec2 t, glm::vec2 s) : translation{t}, scale{s}, frame_ms{0}, flip{0, 0} {}
    bool setAnimation(size_t index) {
        animation_it = animations.begin() + index;
        if (animation_it != animations.end() && !animation_it->frames.empty()) {
            frame_it = animation_it->frames.begin();
            frame_ms = frame_it->duration;
            return true;
        }
        return false;
    }
    Animations animations;
    Animations::iterator animation_it;
    AnimationFrames::iterator frame_it;
    glm::vec2 translation;
    glm::vec2 scale;
    uint16_t frame_ms;
    glm::ivec2 flip;
};

struct SpriteRenderer {
    void create(float width, float height);
    gpu::ShaderProgram *shaderProgram;
    void update(uint16_t dt);
    void render();
    std::list<Sprite> sprites;
};