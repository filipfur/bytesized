#pragma once

#include "gpu.h"
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace animation {

extern bool SETTINGS_LERP;

struct Frame {
    float time;
    union {
        glm::vec3 v3;
        glm::quat q;
    };
};

struct Channel {
    std::vector<Frame> frames;
    std::vector<Frame>::iterator current;
};

struct Animation {
    float startTime;
    float endTime;
    enum ChannelType { CH_TRANSLATION, CH_ROTATION, CH_SCALE };
    std::unordered_map<gpu::Node *, std::array<Channel, 3>> channels;
    std::string_view name;

    void start();
    void stop();
};

struct Player {
    Animation *animation;
    bool paused;
    float time;
};

Animation *createAnimation(const gltf::Animation &animation);
void freeAnimation(Animation *animation);

Player *createPlayer(Animation *animation);
void freePlayer(Player *player);

void animate(float dt);

} // namespace animation