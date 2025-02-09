#pragma once

#include "assets.h"
#include "ecs.h"
#include "geom_primitive.h"
#include "trs.h"
#include <glm/glm.hpp>

struct Pawn {
    TRS *trs;
};
struct Actor : public Pawn {
    glm::vec3 velocity;
    float restitution;
    float mass;
};
struct Collider {
    geom::Geometry *geometry;
    bool transforming;
};
struct Controller {
    static constexpr uint8_t MOVE_FORWARD = 0b00000001;
    static constexpr uint8_t MOVE_LEFT = 0b00000010;
    static constexpr uint8_t MOVE_BACKWARD = 0b00000100;
    static constexpr uint8_t MOVE_RIGHT = 0b00001000;
    static constexpr uint8_t MOVE_JUMP = 0b00010000;

    static constexpr uint8_t STATE_ON_GROUND = 0;
    static constexpr uint8_t STATE_JUMPING = 1;
    static constexpr uint8_t STATE_FALLING = 2;
    static constexpr uint8_t STATE_WALL_HANG = 3;
    static constexpr uint8_t STATE_WALL_JUMP = 4;

    geom::Geometry *floorSense;
    float walkSpeed;
    float jumpTimer;
    uint8_t movement;
    uint8_t state;
    uint8_t jumpCount;
    glm::vec3 up;
    ecs::Entity *lastCollision;
};
struct Gravity {
    float factor;
};
struct SkinAnimated {
    std::vector<animation::Animation *> animations;
    animation::Playback *playback;

    animation::Animation *findAnimation(const char *name) {
        for (auto anim : animations) {
            if (anim->name.compare(name) == 0) {
                return anim;
            }
        }
        return nullptr;
    }

    animation::Playback *playAnimation(const char *name) {
        animation::Animation *anim = findAnimation(name);
        if (anim != playback->animation) {
            printf("Play: %s\n", name);
            playback->animation = anim;
            playback->time = playback->animation->startTime;
            for (auto &channel : anim->channels) {
                // gpu::Node *node = channel.first;
                animation::Channel *ch = &channel.second.at(animation::Animation::CH_TRANSLATION);
                // node->translation = ch->frames.front().v3;
                ch->current = ch->frames.begin();
                ch = &channel.second.at(animation::Animation::CH_ROTATION);
                // node->rotation = ch->frames.front().q;
                ch->current = ch->frames.begin();
                ch = &channel.second.at(animation::Animation::CH_SCALE);
                // node->scale = ch->frames.front().v3;
                ch->current = ch->frames.begin();
            }
        }
        return playback;
    }
};
typedef ecs::Component<Pawn, 0> CPawn;
typedef ecs::Component<Actor, 1> CActor;
typedef ecs::Component<Collider, 2> CCollider;
typedef ecs::Component<Controller, 3> CController;
typedef ecs::Component<Gravity, 4> CGravity;
typedef ecs::Component<SkinAnimated, 5> CSkinAnimated;
