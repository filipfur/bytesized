#include "animation.h"

#include "logging.h"
#include "recycler.hpp"

#define ANIMATION__COUNT 10
#define ANIMATION__PLAYER_COUNT 10
static recycler<animation::Animation, ANIMATION__COUNT> ANIMATIONS = {};
static recycler<animation::Player, ANIMATION__PLAYER_COUNT> PLAYERS = {};
bool animation::SETTINGS_LERP{true};

animation::Animation *animation::createAnimation(const library::Animation &anim) {
    auto rval = ANIMATIONS.acquire();
    rval->startTime = FLT_MAX;
    rval->endTime = -FLT_MAX;
    for (size_t j{0}; j < 96; ++j) {
        const auto &channel = anim.channels[j];
        if (channel.type) {
            float *fp = (float *)channel.sampler->input->data();
            glm::vec3 *v3p = (glm::vec3 *)channel.sampler->output->data();
            glm::vec4 *v4p = (glm::vec4 *)channel.sampler->output->data();
            gpu::Node *targetNode = (gpu::Node *)channel.targetNode->gpuInstance;
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
    for (size_t i{0}; i < PLAYERS.count(); ++i) {
        if (PLAYERS[i].animation == this) {
            return;
        }
    }
    auto player = PLAYERS.acquire();
    player->animation = this;
    player->paused = false;
    player->time = 0.0f;
}

void animation::Animation::stop() {
    animation::Player *player{nullptr};
    for (size_t i{0}; i < PLAYERS.count(); ++i) {
        if (PLAYERS[i].animation == this) {
            player = PLAYERS.data() + 1;
            break;
        }
    }
    if (player) {
        player->animation = nullptr;
        PLAYERS.free(player);
    }
}

animation::Player *animation::createPlayer(animation::Animation *animation) {
    animation::Player *handle = PLAYERS.acquire();
    handle->animation = animation;
    return handle;
};

void animation::freePlayer(animation::Player *player) {
    player->animation = nullptr;
    player->paused = false;
    player->time = 0.0f;
    PLAYERS.free(player);
};

static inline animation::Frame *getFrame(animation::Channel &channel, float time);

void animation::animate(float dt) {
    for (size_t i{0}; i < PLAYERS.count(); ++i) {
        auto &animation = PLAYERS[i];
        if (auto anim = animation.animation) {
            if (animation.paused) {
                continue;
            }
            for (auto &channel : anim->channels) {
                gpu::Node *node = channel.first;
                if (auto tframe = getFrame(channel.second.at(animation::Animation::CH_TRANSLATION),
                                           animation.time)) {
                    if (SETTINGS_LERP) {
                        node->translation =
                            glm::mix(node->translation.data(), tframe->v3, 16.0f * dt);
                    } else {
                        node->translation = tframe->v3;
                    }
                }
                if (auto rframe = getFrame(channel.second.at(animation::Animation::CH_ROTATION),
                                           animation.time)) {
                    if (SETTINGS_LERP) {
                        node->rotation = glm::slerp(node->rotation.data(), rframe->q, 16.0f * dt);
                    } else {
                        node->rotation = rframe->q;
                    }
                }
                if (auto sframe = getFrame(channel.second.at(animation::Animation::CH_SCALE),
                                           animation.time)) {
                    if (SETTINGS_LERP) {
                        node->scale = glm::mix(node->scale.data(), sframe->v3, 16.0f * dt);
                    } else {
                        node->scale = sframe->v3;
                    }
                }
            }
        }
        animation.time += dt;
        if (animation.time > animation.animation->endTime) {
            animation.time -= animation.animation->endTime;
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