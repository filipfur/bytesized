#pragma once

#include "gpu.h"
#include "trs.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <list>
#include <vector>

struct AtlasRegion {
    glm::vec2 bottomLeft;
    glm::vec2 size;
};

template <std::size_t N, std::size_t M> struct Atlas {
    gpu::Texture *texture;
    std::array<std::array<AtlasRegion, N>, M> regions;
};

template <std::size_t cols, std::size_t rows>
inline static constexpr std::array<AtlasRegion, cols * rows> Spritesheet() {
    std::array<AtlasRegion, cols * rows> regions;
    float u = 1.0f / (float)cols;
    float v = 1.0f / (float)rows;
    size_t index = 0;
    for (size_t i{0}; i < cols; ++i) {
        for (size_t j{0}; j < rows; ++j) {
            regions[index++] = {glm::vec2{0.0f + i * u, 0.0f + j * v}, glm::vec2{u, v} * 0.999f};
        }
    }
    return regions;
}

struct AnimationFrame {
    AnimationFrame(const AtlasRegion &r, const uint16_t d) : region{r}, duration{d} {}
    const AtlasRegion &region;
    const uint16_t duration;
};
using AnimationFrames = std::vector<AnimationFrame>;
struct Animation {
    gpu::Texture *texture;
    AnimationFrames frames;
};
using Animations = std::vector<Animation>;

struct Sprite : public TRS {

    Sprite(glm::vec2 t, glm::vec2 s) : TRS{}, frame_ms{0}, flip{0, 0} {
        translation = glm::vec3{t, 0.0f};
        scale = glm::vec3{s, 1.0f};
        hidden = false;
    }
    bool setAnimation(size_t index) {
        animation_it = animations.begin() + index;
        if (animation_it != animations.end() && !animation_it->frames.empty()) {
            frame_it = animation_it->frames.begin();
            frame_ms = frame_it->duration;
            return true;
        }
        return false;
    }

    void setPosition(const glm::vec2 &t) { translation = glm::vec3{t, translation.z}; }
    void move(const glm::vec2 &t) { translation += glm::vec3{t, 0.0f}; }
    glm::vec2 position() const { return {translation.x, translation.y}; }
    void setRotation(float r) { rotation = glm::angleAxis(r, glm::vec3{0.0f, 0.0f, 1.0f}); }
    void rotate(float r) { rotation *= glm::angleAxis(r, glm::vec3{0.0f, 0.0f, 1.0f}); }
    void setSize(const glm::vec2 &s) { scale = glm::vec3{s, 1.0f}; }
    glm::vec2 size() const { return {scale.x, scale.y}; }

    Animations animations;
    Animations::iterator animation_it;
    AnimationFrames::iterator frame_it;
    uint16_t frame_ms;
    glm::ivec2 flip;
    bool hidden;
};

struct SpriteRenderer {
    SpriteRenderer(float width, float height);
    void update(uint16_t dt);
    void render();
    std::list<Sprite> sprites;
    glm::mat4 projection;
};