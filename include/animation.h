#pragma once

#include "gpu.h"
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string_view>
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
    const library::Animation *libraryAnimation;
    float startTime;
    float endTime;
    enum ChannelType { CH_TRANSLATION, CH_ROTATION, CH_SCALE };
    std::unordered_map<gpu::Node *, std::array<Channel, 3>> channels;
    std::string_view name;
    bool looping;

    void retarget(gpu::Node *node);

    void start();
    void stop();
};

struct Playback {
    Animation *animation;
    bool paused;
    float time;
    bool expired() { return time >= animation->endTime; }
};

Animation *createAnimation(const library::Animation &animation, gpu::Node *retargetNode);
void freeAnimation(Animation *animation);

Playback *createPlayback(Animation *animation);
void freePlayback(Playback *playback);

void animate(float dt);

} // namespace animation