#include "animation.h"

#include "logging.h"
#include "recycler.hpp"

#define ANIMATION__COUNT 50
#define ANIMATION__PLAYBACK_COUNT 10
static recycler<animation::Animation, ANIMATION__COUNT> ANIMATIONS = {};
static recycler<animation::Playback, ANIMATION__PLAYBACK_COUNT> PLAYBACKS = {};
bool animation::SETTINGS_LERP{true};

animation::Animation *animation::createAnimation(const library::Animation &anim,
                                                 gpu::Node *retargetNode) {
    auto rval = ANIMATIONS.acquire();
    rval->libraryAnimation = &anim;
    rval->startTime = FLT_MAX;
    rval->endTime = -FLT_MAX;
    rval->looping = true;
    for (size_t j{0}; j < 96; ++j) {
        const auto &channel = anim.channels[j];
        if (channel.type) {
            float *fp = (float *)channel.sampler->input->data();
            glm::vec3 *v3p = (glm::vec3 *)channel.sampler->output->data();
            glm::vec4 *v4p = (glm::vec4 *)channel.sampler->output->data();
            gpu::Node *targetNode = (gpu::Node *)channel.targetNode->gpuInstance;
            if (retargetNode) {
                targetNode = retargetNode->childByName(channel.targetNode->name.c_str());
                assert(targetNode);
            }
            rval->name = anim.name;
            switch (channel.type) {
            case library::Channel::TRANSLATION:
                rval->channels[targetNode][animation::Animation::CH_TRANSLATION].frames.resize(
                    channel.sampler->input->count);
                rval->channels[targetNode][animation::Animation::CH_TRANSLATION].current =
                    rval->channels[targetNode][animation::Animation::CH_TRANSLATION].frames.begin();
                for (size_t k{0}; k < channel.sampler->input->count; ++k) {
                    rval->startTime = std::min(rval->startTime, fp[k]);
                    rval->endTime = std::max(rval->endTime, fp[k]);
                    rval->channels[targetNode][animation::Animation::CH_TRANSLATION]
                        .frames.at(k)
                        .time = fp[k];
                    rval->channels[targetNode][animation::Animation::CH_TRANSLATION]
                        .frames.at(k)
                        .v3.x = v3p[k].x;
                    rval->channels[targetNode][animation::Animation::CH_TRANSLATION]
                        .frames.at(k)
                        .v3.y = v3p[k].y;
                    rval->channels[targetNode][animation::Animation::CH_TRANSLATION]
                        .frames.at(k)
                        .v3.z = v3p[k].z;
                }
                break;
            case library::Channel::ROTATION:
                rval->channels[targetNode][animation::Animation::CH_ROTATION].frames.resize(
                    channel.sampler->input->count);
                rval->channels[targetNode][animation::Animation::CH_ROTATION].current =
                    rval->channels[targetNode][animation::Animation::CH_ROTATION].frames.begin();
                for (size_t k{0}; k < channel.sampler->input->count; ++k) {
                    rval->startTime = std::min(rval->startTime, fp[k]);
                    rval->endTime = std::max(rval->endTime, fp[k]);
                    rval->channels[targetNode][animation::Animation::CH_ROTATION]
                        .frames.at(k)
                        .time = fp[k];
                    rval->channels[targetNode][animation::Animation::CH_ROTATION].frames.at(k).q.x =
                        v4p[k].x;
                    rval->channels[targetNode][animation::Animation::CH_ROTATION].frames.at(k).q.y =
                        v4p[k].y;
                    rval->channels[targetNode][animation::Animation::CH_ROTATION].frames.at(k).q.z =
                        v4p[k].z;
                    rval->channels[targetNode][animation::Animation::CH_ROTATION].frames.at(k).q.w =
                        v4p[k].w;
                }
                break;
            case library::Channel::SCALE:
                rval->channels[targetNode][animation::Animation::CH_SCALE].frames.resize(
                    channel.sampler->input->count);
                rval->channels[targetNode][animation::Animation::CH_SCALE].current =
                    rval->channels[targetNode][animation::Animation::CH_SCALE].frames.begin();
                for (size_t k{0}; k < channel.sampler->input->count; ++k) {
                    rval->startTime = std::min(rval->startTime, fp[k]);
                    rval->endTime = std::max(rval->endTime, fp[k]);
                    rval->channels[targetNode][animation::Animation::CH_SCALE].frames.at(k).time =
                        fp[k];
                    rval->channels[targetNode][animation::Animation::CH_SCALE].frames.at(k).v3.x =
                        v3p[k].x;
                    rval->channels[targetNode][animation::Animation::CH_SCALE].frames.at(k).v3.y =
                        v3p[k].y;
                    rval->channels[targetNode][animation::Animation::CH_SCALE].frames.at(k).v3.z =
                        v3p[k].z;
                }
                break;
            case library::Channel::NONE:
                LOG_ERROR("Unknown animation channel");
                break;
            }
        }
    }
    return rval;
}

void animation::freeAnimation(Animation *animation) { ANIMATIONS.free(animation); }

void animation::Animation::start() {
    for (size_t i{0}; i < PLAYBACKS.count(); ++i) {
        if (PLAYBACKS[i].animation == this) {
            return;
        }
    }
    auto playback = PLAYBACKS.acquire();
    playback->animation = this;
    playback->paused = false;
    playback->time = 0.0f;
}

void animation::Animation::stop() {
    animation::Playback *playback{nullptr};
    for (size_t i{0}; i < PLAYBACKS.count(); ++i) {
        if (PLAYBACKS[i].animation == this) {
            playback = PLAYBACKS.data() + 1;
            break;
        }
    }
    if (playback) {
        playback->animation = nullptr;
        PLAYBACKS.free(playback);
    }
}

animation::Playback *animation::createPlayback(animation::Animation *animation) {
    animation::Playback *handle = PLAYBACKS.acquire();
    handle->animation = animation;
    return handle;
};

void animation::freePlayback(animation::Playback *playback) {
    playback->animation = nullptr;
    playback->paused = false;
    playback->time = 0.0f;
    PLAYBACKS.free(playback);
};

static inline animation::Frame *getFrame(animation::Channel &channel, float time);

void animation::animate(float dt) {
    for (size_t i{0}; i < PLAYBACKS.count(); ++i) {
        auto &playback = PLAYBACKS[i];
        if (auto anim = playback.animation) {
            if (playback.paused) {
                continue;
            }
            for (auto &channel : anim->channels) {
                gpu::Node *node = channel.first;
                if (auto tframe = getFrame(channel.second.at(animation::Animation::CH_TRANSLATION),
                                           playback.time)) {
                    if (SETTINGS_LERP) {
                        node->translation =
                            glm::mix(node->translation.data(), tframe->v3, 16.0f * dt);
                    } else {
                        node->translation = tframe->v3;
                    }
                }
                if (auto rframe = getFrame(channel.second.at(animation::Animation::CH_ROTATION),
                                           playback.time)) {
                    if (SETTINGS_LERP) {
                        node->rotation = glm::slerp(node->rotation.data(), rframe->q, 16.0f * dt);
                    } else {
                        node->rotation = rframe->q;
                    }
                }
                if (auto sframe = getFrame(channel.second.at(animation::Animation::CH_SCALE),
                                           playback.time)) {
                    if (SETTINGS_LERP) {
                        node->scale = glm::mix(node->scale.data(), sframe->v3, 16.0f * dt);
                    } else {
                        node->scale = sframe->v3;
                    }
                }
            }
        }
        playback.time += dt;
        if (playback.time > playback.animation->endTime) {
            if (playback.animation->looping) {
                playback.time =
                    playback.animation->startTime + (playback.time - playback.animation->endTime);
            } else {
                playback.time = std::min(playback.time, playback.animation->endTime);
            }
        }
    }
}

static inline animation::Frame *getFrame(animation::Channel &channel, float time) {
    if (channel.current == channel.frames.end()) {
        if (channel.frames.empty()) {
            return nullptr;
        }
        channel.current = channel.frames.begin();
    }
    while (channel.current->time < time) {
        if (channel.current == (channel.frames.end() - 1)) {
            break;
        }
        ++channel.current;
    }
    if (channel.current == (channel.frames.end() - 1)) {
        float d1 = time - channel.frames.begin()->time;
        float d2 = time - channel.current->time;
        // check if we wrapped
        if (glm::abs(d1) < glm::abs(d2)) {
            channel.current = channel.frames.begin();
        }
    }
    if (channel.current != channel.frames.end()) {
        return &(*channel.current);
    }
    return nullptr;
}