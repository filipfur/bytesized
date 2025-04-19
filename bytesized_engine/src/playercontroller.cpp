#include "playercontroller.h"

#include "geom_collision.h"
#include "playerinput.h"
#include "primer.h"
#include "window.h"

static constexpr float JUMP_HEIGHT = 0.8f;
static constexpr float JUMP_TIME = 0.3f;
static constexpr float GRAVITY = (-2.0f * JUMP_HEIGHT) / (JUMP_TIME * JUMP_TIME);
static constexpr float JUMP_V0 = -GRAVITY * JUMP_TIME;

static geom::Collision collision;

static void _collisionResponse(Actor &actor, const glm::vec3 &collisionNormal,
                               float penetrationDepth);
static bool _handleCollision(Actor &actor_a, Collider &collider_a, TRS *trs_b, Collider &collider_b,
                             Actor *actor_b);

static void _firstPersonControllerMovement(Actor &actor, Controller &controller, float moveSpeed,
                                           float dt) {
    glm::vec3 v0{actor.velocity.x, 0.0f, actor.velocity.z};
    glm::vec3 v1{0.0f, 0.0f, 0.0f};
    if (controller.movement & MOVE_FORWARD) {
        v1 += (actor.trs->r() * primer::FORWARD);
    } else if (controller.movement & MOVE_BACKWARD) {
        v1 += (actor.trs->r() * -primer::FORWARD);
    }
    if (controller.movement & MOVE_LEFT) {
        v1 += (actor.trs->r() * -primer::AXIS_X);
        v1 = glm::normalize(v1);
    } else if (controller.movement & MOVE_RIGHT) {
        v1 += (actor.trs->r() * primer::AXIS_X);
        v1 = glm::normalize(v1);
    }
    float a = moveSpeed * dt;
    actor.velocity.x = glm::mix(v0.x, v1.x * controller.walkSpeed, a);
    actor.velocity.z = glm::mix(v0.z, v1.z * controller.walkSpeed, a);
}

static void _thirdPersonControllerMovement(Actor &actor, Controller &controller, float moveSpeed,
                                           float retardation, float dt) {
    bool moving = controller.movement & (MOVE_FORWARD | MOVE_RIGHT | MOVE_LEFT | MOVE_BACKWARD);
    if (moving) {
        glm::vec3 v0{actor.velocity.x, 0.0f, actor.velocity.z};
        glm::vec3 v1 = (actor.trs->r() * primer::FORWARD) * controller.walkSpeed;
        float a = moveSpeed * dt;
        actor.velocity.x = glm::mix(v0.x, v1.x, a);
        actor.velocity.z = glm::mix(v0.z, v1.z, a);
    } else {
        float a = retardation * dt;
        float b = 1.0f - a;
        actor.velocity.x = b * actor.velocity.x;
        actor.velocity.z = b * actor.velocity.z;
    }
}

void PlayerController_firstPersonPlayerController(Character &character, float dt) {
    ecs::Entity *entity = (ecs::Entity *)character.node->entity;
    auto &controller = *character.ctrl;
    auto &actor = *character.actor;
    controller.up = primer::UP;
    switch (controller.state) {
    case Controller::STATE_ON_GROUND:
        _firstPersonControllerMovement(actor, controller, 6.0f, dt);
        if (!ecs::System<CPawn, CCollider>::until(
                [&](ecs::Entity *entity_b, Pawn &pawn_b, Collider &collider_b) {
                    (void)entity_b;
                    controller.floorSense->trs = actor.trs;
                    collider_b.geometry->trs = collider_b.transforming ? pawn_b.trs : nullptr;
                    // TODO: Rotate up vector according to trs of actor
                    if (geom::collides(*controller.floorSense, *collider_b.geometry, &collision) &&
                        glm::dot(collision.normal, glm::vec3{0.0f, 1.0f, 0.0f}) > 0.5f) {
                        return true;
                    }
                    return false;
                })) {
            controller.state = Controller::STATE_FALLING;
            CGravity::attach(entity);
            --controller.jumpCount;
        }
        if (controller.jumpCount && controller.jumpTimer < FLT_EPSILON &&
            controller.movement & MOVE_JUMP) {
            actor.velocity.y = JUMP_V0;
            controller.state = Controller::STATE_JUMPING;
            CGravity::attach(entity);
            controller.movement &= ~MOVE_JUMP;
        }
        break;
    }
}

void PlayerController_thirdPersonPlayerController(Character &character, float dt) {
    ecs::Entity *entity = (ecs::Entity *)character.node->entity;
    auto &controller = *character.ctrl;
    auto &actor = *character.actor;
    controller.up = primer::UP;
    if (controller.jumpTimer > 0) {
        controller.jumpTimer -= dt;
    }
    static constexpr float TIME_BETWEEN_JUMPS = 0.2f;
    static constexpr float WALL_JUMP_TURN_TIME = 0.2f;
    switch (controller.state) {
    case Controller::STATE_ON_GROUND:
        _thirdPersonControllerMovement(actor, controller, 6.0f, 12.0f, dt);
        if (!ecs::System<CPawn, CCollider>::until(
                [&](ecs::Entity *entity_b, Pawn &pawn_b, Collider &collider_b) {
                    (void)entity_b;
                    controller.floorSense->trs = actor.trs;
                    collider_b.geometry->trs = collider_b.transforming ? pawn_b.trs : nullptr;
                    // TODO: Rotate up vector according to trs of actor
                    if (geom::collides(*controller.floorSense, *collider_b.geometry, &collision) &&
                        glm::dot(collision.normal, glm::vec3{0.0f, 1.0f, 0.0f}) > 0.5f) {
                        return true;
                    }
                    return false;
                })) {
            controller.state = Controller::STATE_FALLING;
            CGravity::attach(entity);
            --controller.jumpCount;
        }
        if (controller.jumpCount && controller.jumpTimer < FLT_EPSILON &&
            controller.movement & MOVE_JUMP) {
            actor.velocity.y = JUMP_V0;
            controller.state = Controller::STATE_JUMPING;
            controller.jumpTimer = TIME_BETWEEN_JUMPS;
            CGravity::attach(entity);
            --controller.jumpCount;
            controller.movement &= ~MOVE_JUMP;
        }
        break;
    case Controller::STATE_WALL_HANG:
        if (controller.jumpTimer < FLT_EPSILON && controller.movement & MOVE_JUMP) {
            glm::vec3 n = actor.trs->r() * glm::vec3{0.0f, 0.0f, 1.0f};
            actor.velocity += n * 3.0f + glm::vec3{0.0f, JUMP_V0, 0.0f};
            controller.state = Controller::STATE_WALL_JUMP;
            controller.jumpTimer = WALL_JUMP_TURN_TIME;
            controller.jumpCount = 0;
            controller.movement &= ~MOVE_JUMP;
            actor.trs->translation += n * 0.1f;
        }
        break;
    case Controller::STATE_WALL_JUMP:
        actor.trs->euler += {0.0f, 180.0f * dt / WALL_JUMP_TURN_TIME, 0.0f};
        if (controller.jumpTimer < FLT_EPSILON) {
            controller.state = Controller::STATE_FALLING;
        }
        break;
    case Controller::STATE_FALLING:
        _thirdPersonControllerMovement(actor, controller, 0.5f, 1.0f, dt);
        if (controller.jumpCount && controller.jumpTimer < FLT_EPSILON &&
            controller.movement & MOVE_JUMP) {
            actor.velocity.y = JUMP_V0 * 0.8f;
            controller.state = Controller::STATE_JUMPING;
            controller.jumpTimer = TIME_BETWEEN_JUMPS;
            --controller.jumpCount;
            controller.movement &= ~MOVE_JUMP;
        }
        break;
    case Controller::STATE_JUMPING:
        _thirdPersonControllerMovement(actor, controller, 0.5f, 1.0f, dt);
        if (actor.velocity.y < 0 || controller.jumpTimer < FLT_EPSILON) {
            controller.state = Controller::STATE_FALLING;
        }
        break;
    }
    controller.lastCollision = nullptr;
}

void PlayerController_updateActorGravity(float dt) {
    ecs::System<CActor, CGravity>::for_each(
        [dt](ecs::Entity *entity, Actor &actor, Gravity &gravity) {
            (void)entity;
            static const float maxFallSpeed{100'000.0f};
            if (actor.velocity.y > -maxFallSpeed) {
                // heavy landing
                // (actor.velocity.y < 0 ? (GRAVITY + actor.velocity.y * 2.0f) : GRAVITY)
                actor.velocity.y += GRAVITY * dt * gravity.factor;
            }
        });
}

void PlayerController_updateActorMovement(float dt) {
    ecs::System<CActor>::for_each([dt](ecs::Entity *entity, Actor &actor) {
        (void)entity;
        // apply velocity
        actor.trs->translation += actor.velocity * dt;
    });
}

void PlayerController_updatePlatformCollisions() {
    // Actor x Pawn collision (Dynamic-Static)
    ecs::System<CActor, CCollider>::for_each([](ecs::Entity *entity_a, Actor &actor_a,
                                                Collider &collider_a) {
        ecs::System<CPawn, CCollider>::for_each([&](ecs::Entity *entity_b, Pawn &pawn_b,
                                                    Collider &collider_b) {
            if (_handleCollision(actor_a, collider_a, pawn_b.trs, collider_b, nullptr)) {
                // printf("%p collision: %f %s\n", (void *)pawn_b.trs,
                // _collision.penetrationDepth,
                //        glm::to_string(_collision.normal).c_str());
                if (auto ctrl = CController::get_pointer(entity_a)) {
                    ctrl->lastCollision = entity_b;
                    float d = glm::dot(collision.normal, primer::UP);
                    if (d > 0.5f) {
                        // floor collision
                        actor_a.velocity.y = 0.0f;
                        actor_a.trs->translation += collision.normal * 0.0005f;
                        CGravity::detach(entity_a);
                        ctrl->state = Controller::STATE_ON_GROUND;
                        ctrl->jumpCount = 2;
                        ctrl->jumpTimer = 0.0f;
                        ctrl->up = collision.normal;
                    } else if (d < -0.5f) {
                        // bump head
                        actor_a.velocity.y = 0.0f;
                    } else {
                        // lateral collision
#ifdef WALL_JUMPING
                        glm::vec3 v;
                        float theta;
                        switch (ctrl->state) {
                        case Controller::STATE_JUMPING:
                        case Controller::STATE_FALLING:
                        case Controller::STATE_WALL_JUMP:
                            actor_a.velocity.x = actor_a.velocity.z = 0.0f;
                            v = actor_a.trs->r() * glm::vec3{0.0f, 0.0f, 1.0f};
                            theta = primer::angleBetween(
                                glm::normalize(glm::vec2{v.x, v.z}),
                                glm::normalize(glm::vec2{collision.normal.x, collision.normal.z}));
                            actor_a.trs->euler += {0.0f, glm::degrees(-theta), 0.0f};
                            ctrl->state = Controller::STATE_WALL_HANG;
                            break;
                        default:
                            break;
                        }
#endif
                    }
                    return;
                }
                if (glm::dot(actor_a.velocity, actor_a.velocity) < 0.0001f) {
                    CPawn::attach(entity_a);
                    CPawn::get(entity_a).trs = actor_a.trs;
                    CActor::detach(entity_a);
                    return;
                }
            }
        });
    });
}

void PlayerController_updateKineticCollisions() {
    // Actor x Actor collision (Dynamic-Dynamic)
    // ecs::System<CActor, CCollider>::combine(
    //     [](ecs::Entity *entity_a, Actor &actor_a, Collider &collider_a, ecs::Entity *entity_b,
    //        Actor &actor_b, Collider &collider_b) {
    //         if (_handleCollision(actor_a, collider_a, actor_b.trs, collider_b,
    //                              &actor_b)) {
    //             /*CActor::detach(entity_a);
    //             CActor::detach(entity_b);*/
    //         }
    //     },
    //     false);
}

void PlayerController_setPlayerAnimation(Character &character) {
    auto *skin = character.node->getSkin();
    switch (character.ctrl->state) {
    case Controller::STATE_FALLING:
        skin->playAnimation("Falling");
        break;
    case Controller::STATE_JUMPING:
        if (character.ctrl->jumpCount) {
            skin->playAnimation("Jumping");
        } else {
            skin->playAnimation("DoubleJump");
        }
        // skin->playAnimation("Jumping");
        break;
    case Controller::STATE_ON_GROUND:
        if ((character.ctrl->movement & (MOVE_FORWARD | MOVE_BACKWARD | MOVE_RIGHT | MOVE_LEFT)) >
            0) {
            skin->playAnimation("Running");
        } /* else if (character.ctrl->movement & (MOVE_LEFT)) {
             skin->playAnimation("StrafingLeft");
         } else if (character.ctrl->movement & (MOVE_RIGHT)) {
             skin->playAnimation("StrafingRight");
         } */
        else {
            skin->playAnimation("Idle");
        }
        break;
    case Controller::STATE_WALL_HANG:
        skin->playAnimation("WallHang");
        break;
    case Controller::STATE_WALL_JUMP:
        skin->playAnimation("Jumping");
        break;
    }
}

void PlayerController_cameraCollision(Camera &camera) {
    camera.targetView.distance = glm::max(6.0f - camera.currentView.pitch * 0.06f, 0.1f);

    float tmin = 0.0f;
    float tmax = 0.0f;

    glm::vec3 targetOrientation =
        glm::angleAxis(glm::radians(camera.targetView.yaw), primer::UP) *
        glm::angleAxis(glm::radians(camera.targetView.pitch), primer::RIGHT) * primer::FORWARD;

    ecs::System<CPawn, CCollider>::for_each([&](ecs::Entity *e, Pawn &pawn, Collider &collider) {
        (void)e;
        (void)pawn;
        tmin = 0.0f;
        tmax = 6.0f;
        if (collider.geometry->rayIntersect(camera.targetView.center, -targetOrientation, tmin,
                                            tmax)) {
            if (tmin > 0.01f && tmin < camera.targetView.distance) {
                camera.targetView.distance = glm::max(tmin - 0.1f, 0.1f);
            }
        }
    });
}

void PlayerController_firstPersonCameraFollow(Camera &camera, Character &character, float dt) {
    glm::vec3 e = character.node->euler.data();
    character.node->euler = {
        0.0f,
        glm::mix(e.y, camera.currentView.yaw, 50.0f * dt),
        0.0f,
    };
    switch (character.ctrl->state) {
    default:
        camera.targetView.center =
            character.node
                ? (character.node->t() + glm::vec3{0.0f, 0.2f, 0.0f} + camera.orientation() * 0.1f)
                : glm::vec3{0.0f, 0.0f, 0.0f};
        camera.currentView.center = camera.targetView.center;
        break;
    }
}

void PlayerController_thirdPersonCameraFollow(Camera &camera, Character &character, float dt) {
    float f = character.actor->velocity.x * character.actor->velocity.x +
              character.actor->velocity.z * character.actor->velocity.z;
    glm::vec3 e = character.node->euler.data();
    float offs = 0.0f;
    if (character.ctrl->state == Controller::STATE_ON_GROUND) {
        if (character.ctrl->movement & MOVE_LEFT) {
            offs = 90.0f;
        } else if (character.ctrl->movement & MOVE_RIGHT) {
            offs = -90.0f;
        } else if (character.ctrl->movement & MOVE_BACKWARD) {
            offs = 180.0f;
        }
    } else {
        f = 0.01f;
    }
    character.node->euler = {
        0.0f,
        glm::mix(e.y, camera.targetView.yaw + offs, std::min(f, 1.0f) * 50.0f * dt),
        0.0f,
    };
    switch (character.ctrl->state) {
    default:
        camera.targetView.center =
            character.node
                ? (character.node->t() + glm::vec3{0.0f, 1.0f, 0.0f} + camera.orientation())
                : glm::vec3{0.0f, 0.0f, 0.0f};
        break;
    }
}

static void _collisionResponse(Actor &actor, const glm::vec3 &collisionNormal,
                               float penetrationDepth) {
    actor.trs->translation += collisionNormal * penetrationDepth;
    if (std::abs(glm::dot(collisionNormal, primer::UP)) < 0.7f) {
        actor.velocity = glm::reflect(actor.velocity, collisionNormal) * actor.restitution;
    } else {
        actor.velocity.y = -actor.velocity.y * actor.restitution;
    }
}

static bool _handleCollision(Actor &actor_a, Collider &collider_a, TRS *trs_b, Collider &collider_b,
                             Actor *actor_b) {
    collider_a.geometry->trs = collider_a.transforming ? actor_a.trs : nullptr;
    collider_b.geometry->trs = collider_b.transforming ? trs_b : nullptr;
    if (geom::collides(*collider_a.geometry, *collider_b.geometry, &collision)) {
        _collisionResponse(actor_a, collision.normal, collision.penetrationDepth);
        if (actor_b) {
            _collisionResponse(actor_a, -collision.normal, collision.penetrationDepth);
        }
        return true;
    }
    return false;
}