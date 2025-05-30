#pragma once

#include "bytesized_info.h"

#ifdef BYTESIZED_USE_SKINNING
namespace gpu {

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
    std::unordered_map<struct Node *, std::array<Channel, 3>> channels;
    std::string_view name;
    bool looping;

    void start();
    void stop();
};

struct Playback {
    Animation *animation;
    bool paused;
    float time;
    bool expired() { return time >= animation->endTime; }
};

Animation *createAnimation(const library::Animation &animation, struct Node *retargetNode);
void freeAnimation(Animation *animation);

Playback *createPlayback(Animation *animation);
void freePlayback(Playback *playback);

void animate(float dt);

struct Skin {
    const library::Skin *librarySkin;
    std::vector<struct Node *> joints;

    std::vector<gpu::Animation *> animations;
    gpu::Playback *playback;

    gpu::Animation *findAnimation(const char *name);
    gpu::Playback *playAnimation(const char *name);
};

} // namespace gpu

#endif
