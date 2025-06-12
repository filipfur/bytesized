#ifdef BYTESIZED_USE_SKINNING

#include "gpu.h"
#include "logging.h"

static recycler<gpu::Skin, BYTESIZED_SKIN_COUNT> SKINS = {};
static recycler<gpu::Animation, BYTESIZED_ANIMATION_COUNT> ANIMATIONS = {};
static recycler<gpu::Playback, BYTESIZED_PLAYBACK_COUNT> PLAYBACKS = {};

gpu::Animation *gpu::createAnimation(const library::Animation &anim, gpu::Node *retargetNode) {
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
                rval->channels[targetNode][gpu::Animation::CH_TRANSLATION].frames.resize(
                    channel.sampler->input->count);
                rval->channels[targetNode][gpu::Animation::CH_TRANSLATION].current =
                    rval->channels[targetNode][gpu::Animation::CH_TRANSLATION].frames.begin();
                for (size_t k{0}; k < channel.sampler->input->count; ++k) {
                    rval->startTime = std::min(rval->startTime, fp[k]);
                    rval->endTime = std::max(rval->endTime, fp[k]);
                    rval->channels[targetNode][gpu::Animation::CH_TRANSLATION].frames.at(k).time =
                        fp[k];
                    rval->channels[targetNode][gpu::Animation::CH_TRANSLATION].frames.at(k).v3.x =
                        v3p[k].x;
                    rval->channels[targetNode][gpu::Animation::CH_TRANSLATION].frames.at(k).v3.y =
                        v3p[k].y;
                    rval->channels[targetNode][gpu::Animation::CH_TRANSLATION].frames.at(k).v3.z =
                        v3p[k].z;
                }
                break;
            case library::Channel::ROTATION:
                rval->channels[targetNode][gpu::Animation::CH_ROTATION].frames.resize(
                    channel.sampler->input->count);
                rval->channels[targetNode][gpu::Animation::CH_ROTATION].current =
                    rval->channels[targetNode][gpu::Animation::CH_ROTATION].frames.begin();
                for (size_t k{0}; k < channel.sampler->input->count; ++k) {
                    rval->startTime = std::min(rval->startTime, fp[k]);
                    rval->endTime = std::max(rval->endTime, fp[k]);
                    rval->channels[targetNode][gpu::Animation::CH_ROTATION].frames.at(k).time =
                        fp[k];
                    rval->channels[targetNode][gpu::Animation::CH_ROTATION].frames.at(k).q.x =
                        v4p[k].x;
                    rval->channels[targetNode][gpu::Animation::CH_ROTATION].frames.at(k).q.y =
                        v4p[k].y;
                    rval->channels[targetNode][gpu::Animation::CH_ROTATION].frames.at(k).q.z =
                        v4p[k].z;
                    rval->channels[targetNode][gpu::Animation::CH_ROTATION].frames.at(k).q.w =
                        v4p[k].w;
                }
                break;
            case library::Channel::SCALE:
                rval->channels[targetNode][gpu::Animation::CH_SCALE].frames.resize(
                    channel.sampler->input->count);
                rval->channels[targetNode][gpu::Animation::CH_SCALE].current =
                    rval->channels[targetNode][gpu::Animation::CH_SCALE].frames.begin();
                for (size_t k{0}; k < channel.sampler->input->count; ++k) {
                    rval->startTime = std::min(rval->startTime, fp[k]);
                    rval->endTime = std::max(rval->endTime, fp[k]);
                    rval->channels[targetNode][gpu::Animation::CH_SCALE].frames.at(k).time = fp[k];
                    rval->channels[targetNode][gpu::Animation::CH_SCALE].frames.at(k).v3.x =
                        v3p[k].x;
                    rval->channels[targetNode][gpu::Animation::CH_SCALE].frames.at(k).v3.y =
                        v3p[k].y;
                    rval->channels[targetNode][gpu::Animation::CH_SCALE].frames.at(k).v3.z =
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

size_t gpu::skinningBufferSize() { return sizeof(SKINS) + sizeof(ANIMATIONS) + sizeof(PLAYBACKS); }

#define PRINT_USAGE(var)                                                                           \
    do {                                                                                           \
        printf("%s: %zu / %zu\n", #var, var.count(), var.size());                                  \
    } while (0);

void gpu::printSkinningUsages() {
    PRINT_USAGE(SKINS);
    PRINT_USAGE(ANIMATIONS);
    PRINT_USAGE(PLAYBACKS);
}

gpu::Skin *gpu::createSkin(const library::Skin &librarySkin) {
    gpu::Skin *skin = SKINS.acquire();
    skin->librarySkin = &librarySkin;
    return skin;
}

void gpu::freeSkin(gpu::Skin *skin) {
    skin->animations.clear();
    skin->playback = nullptr;
    skin->joints.clear();
    skin->librarySkin = nullptr;
    SKINS.free(skin);
}

void gpu::freeAnimation(Animation *animation) {
    animation->startTime = 0;
    animation->endTime = 0;
    animation->name = {};
    animation->channels.clear();
    animation->libraryAnimation = nullptr;
    animation->looping = true;
    ANIMATIONS.free(animation);
}

gpu::Animation *gpu::Skin::findAnimation(const char *name) {
    for (auto anim : animations) {
        if (anim->name.compare(name) == 0) {
            return anim;
        }
    }
    return nullptr;
}

gpu::Playback *gpu::Skin::playAnimation(const char *name) {
    gpu::Animation *anim = findAnimation(name);
    if (anim != playback->animation) {
        playback->animation = anim;
        playback->time = playback->animation->startTime;
        for (auto &channel : anim->channels) {
            gpu::Channel *ch = &channel.second.at(gpu::Animation::CH_TRANSLATION);
            ch->current = ch->frames.begin();
            ch = &channel.second.at(gpu::Animation::CH_ROTATION);
            ch->current = ch->frames.begin();
            ch = &channel.second.at(gpu::Animation::CH_SCALE);
            ch->current = ch->frames.begin();
        }
    }
    return playback;
}

void gpu::Animation::start() {
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

void gpu::Animation::stop() {
    gpu::Playback *playback{nullptr};
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

gpu::Playback *gpu::createPlayback(gpu::Animation *animation) {
    gpu::Playback *handle = PLAYBACKS.acquire();
    handle->animation = animation;
    return handle;
};

void gpu::freePlayback(gpu::Playback *playback) {
    playback->animation = nullptr;
    playback->paused = false;
    playback->time = 0.0f;
    PLAYBACKS.free(playback);
};

static inline gpu::Frame *getFrame(gpu::Channel &channel, float time);

static constexpr bool _lerpBetweenKeyFrames = true;
void gpu::animate(float dt) {
    for (size_t i{0}; i < PLAYBACKS.count(); ++i) {
        auto &playback = PLAYBACKS[i];
        if (auto anim = playback.animation) {
            if (playback.paused) {
                continue;
            }
            for (auto &channel : anim->channels) {
                gpu::Node *node = channel.first;
                if (auto tframe = getFrame(channel.second.at(gpu::Animation::CH_TRANSLATION),
                                           playback.time)) {
                    if (_lerpBetweenKeyFrames) {
                        node->translation =
                            glm::mix(node->translation.data(), tframe->v3, 16.0f * dt);
                    } else {
                        node->translation = tframe->v3;
                    }
                }
                if (auto rframe =
                        getFrame(channel.second.at(gpu::Animation::CH_ROTATION), playback.time)) {
                    if (_lerpBetweenKeyFrames) {
                        node->rotation = glm::slerp(node->rotation.data(), rframe->q, 16.0f * dt);
                    } else {
                        node->rotation = rframe->q;
                    }
                }
                if (auto sframe =
                        getFrame(channel.second.at(gpu::Animation::CH_SCALE), playback.time)) {
                    if (_lerpBetweenKeyFrames) {
                        node->scale = glm::mix(node->scale.data(), sframe->v3, 16.0f * dt);
                    } else {
                        node->scale = sframe->v3;
                    }
                }
            }
            playback.time += dt;
            if (playback.time > playback.animation->endTime) {
                if (playback.animation->looping) {
                    playback.time = playback.animation->startTime +
                                    (playback.time - playback.animation->endTime);
                } else {
                    playback.time = std::min(playback.time, playback.animation->endTime);
                }
            }
        }
    }
}

static inline gpu::Frame *getFrame(gpu::Channel &channel, float time) {
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

#endif