#pragma once

#include "assets.h"
#include "ecs.h"
#include "geom_primitive.h"
#include "trs.h"
#include <functional>
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
struct Billboard {
    uint32_t id;
    gpu::Texture *texture;
};
struct Interactable {
    static constexpr auto noAction = [](float, gpu::Node *, gpu::Node *) {};
    std::function<void(float, gpu::Node *, gpu::Node *)> inRadius;
    std::function<void(float, gpu::Node *, gpu::Node *)> interaction;
    float radii;
};
typedef ecs::Component<Pawn, 0> CPawn;
typedef ecs::Component<Actor, 1> CActor;
typedef ecs::Component<Collider, 2> CCollider;
typedef ecs::Component<Controller, 3> CController;
typedef ecs::Component<Gravity, 4> CGravity;
typedef ecs::Component<Billboard, 5> CBillboard;
typedef ecs::Component<Interactable, 6> CInteractable;

inline static void __freeEntity(ecs::Entity **entity_ptr) {
    ecs::Entity *entity = *entity_ptr;
    if (entity) {
        if (auto *pawn = CPawn::get_pointer(entity)) {
            pawn->trs = nullptr;
        }
        if (auto *actor = CActor::get_pointer(entity)) {
            actor->trs = nullptr;
        }
        if (auto *collider = CCollider::get_pointer(entity)) {
            geom::free(collider->geometry);
            collider->geometry = nullptr;
        }
        if (auto *controller = CController::get_pointer(entity)) {
            geom::free(controller->floorSense);
            controller->floorSense = nullptr;
            controller->lastCollision = nullptr;
        }
        if (auto *billboard = CBillboard::get_pointer(entity)) {
            billboard->texture = nullptr;
        }
        ecs::dispose_entity(entity);
        *entity_ptr = nullptr;
    }
}