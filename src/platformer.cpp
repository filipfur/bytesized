#include "platformer.h"

#include "geom_collision.h"
#include "primer.h"
#include "window.h"

static constexpr float JUMP_HEIGHT = 2.2f;
static constexpr float JUMP_TIME = 0.42f;
static constexpr float GRAVITY = (-2.0f * JUMP_HEIGHT) / (JUMP_TIME * JUMP_TIME);
static constexpr float JUMP_V0 = -GRAVITY * JUMP_TIME;

static geom::Collision collision;

static void _collisionResponse(Actor &actor, const glm::vec3 &collisionNormal,
                               float penetrationDepth);
static bool _handleCollision(Actor &actor_a, Collider &collider_a, TRS *trs_b, Collider &collider_b,
                             Actor *actor_b);

static void _updateControllerMovement(Actor &actor, Controller &controller, float moveSpeed,
                                      float retardation, float dt) {
    bool moving = controller.movement &
                  (Controller::MOVE_FORWARD | Controller::MOVE_RIGHT | Controller::MOVE_LEFT);
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

void Platformer_updatePlayerController(Character &character, float dt) {
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
        _updateControllerMovement(actor, controller, 6.0f, 12.0f, dt);
        if (!ecs::System<CPawn, CCollider>::until(
                [&](ecs::Entity *entity_b, Pawn &pawn_b, Collider &collider_b) {
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
            controller.movement & Controller::MOVE_JUMP) {
            actor.velocity.y = JUMP_V0;
            controller.state = Controller::STATE_JUMPING;
            controller.jumpTimer = TIME_BETWEEN_JUMPS;
            CGravity::attach(entity);
            --controller.jumpCount;
            controller.movement &= ~Controller::MOVE_JUMP;
        }
        break;
    case Controller::STATE_WALL_HANG:
        if (controller.jumpTimer < FLT_EPSILON && controller.movement & Controller::MOVE_JUMP) {
            glm::vec3 n = actor.trs->r() * glm::vec3{0.0f, 0.0f, 1.0f};
            actor.velocity += n * 3.0f + glm::vec3{0.0f, JUMP_V0, 0.0f};
            controller.state = Controller::STATE_WALL_JUMP;
            controller.jumpTimer = WALL_JUMP_TURN_TIME;
            controller.jumpCount = 0;
            controller.movement &= ~Controller::MOVE_JUMP;
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
        _updateControllerMovement(actor, controller, 0.5f, 1.0f, dt);
        if (controller.jumpCount && controller.jumpTimer < FLT_EPSILON &&
            controller.movement & Controller::MOVE_JUMP) {
            actor.velocity.y = JUMP_V0 * 0.8f;
            controller.state = Controller::STATE_JUMPING;
            controller.jumpTimer = TIME_BETWEEN_JUMPS;
            --controller.jumpCount;
            controller.movement &= ~Controller::MOVE_JUMP;
        }
        break;
    case Controller::STATE_JUMPING:
        _updateControllerMovement(actor, controller, 0.5f, 1.0f, dt);
        if (actor.velocity.y < 0 || controller.jumpTimer < FLT_EPSILON) {
            controller.state = Controller::STATE_FALLING;
        }
        break;
    }
    controller.lastCollision = nullptr;
}

void Platformer_updateActorGravity(float dt) {
    ecs::System<CActor, CGravity>::for_each(
        [dt](ecs::Entity *entity, Actor &actor, Gravity &gravity) {
            static const float maxFallSpeed{100'000.0f};
            if (actor.velocity.y > -maxFallSpeed) {
                actor.velocity.y +=
                    (actor.velocity.y < 0 ? (GRAVITY + actor.velocity.y * 2.0f) : GRAVITY) * dt *
                    gravity.factor;
            }
        });
}

void Platformer_updateActorMovement(float dt) {
    ecs::System<CActor>::for_each([dt](ecs::Entity *entity, Actor &actor) {
        // apply velocity
        actor.trs->translation += actor.velocity * dt;
    });
}

void Platformer_updatePlatformCollisions(float dt) {
    // Actor x Pawn collision (Dynamic-Static)
    ecs::System<CActor, CCollider>::for_each([](ecs::Entity *entity_a, Actor &actor_a,
                                                Collider &collider_a) {
        ecs::System<CPawn, CCollider>::until([&](ecs::Entity *entity_b, Pawn &pawn_b,
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
                        if (ctrl->state == Controller::STATE_JUMPING ||
                            ctrl->state == Controller::STATE_FALLING ||
                            ctrl->state == Controller::STATE_WALL_JUMP) {
                            actor_a.velocity.x = actor_a.velocity.z = 0.0f;
                            glm::vec3 v = actor_a.trs->r() * glm::vec3{0.0f, 0.0f, 1.0f};
                            float theta = primer::angleBetween(
                                glm::normalize(glm::vec2{v.x, v.z}),
                                glm::normalize(glm::vec2{collision.normal.x, collision.normal.z}));
                            actor_a.trs->euler += {0.0f, glm::degrees(-theta), 0.0f};
                            ctrl->state = Controller::STATE_WALL_HANG;
                        }
                    }
                    return true;
                }
                if (glm::dot(actor_a.velocity, actor_a.velocity) < 0.01f) {
                    CPawn::attach(entity_a);
                    CPawn::get(entity_a).trs = actor_a.trs;
                    CActor::detach(entity_a);
                    return true;
                }
            }
            return false;
        });
    });
}

void Platformer_updateKineticCollisions(float dt) {
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

void Platformer_setPlayerAnimation(Character &character) {
    ecs::Entity *entity = (ecs::Entity *)character.node->entity;
    auto &skinAnim = CSkinAnimated::get(entity);
    switch (character.ctrl->state) {
    case Controller::STATE_FALLING:
        skinAnim.playAnimation("Falling");
        break;
    case Controller::STATE_JUMPING:
        if (character.ctrl->jumpCount) {
            skinAnim.playAnimation("Jumping");
        } else {
            skinAnim.playAnimation("DoubleJump");
        }
        // skinAnim.playAnimation("Jumping");
        break;
    case Controller::STATE_ON_GROUND:
        if ((character.ctrl->movement & (Controller::MOVE_FORWARD | Controller::MOVE_BACKWARD |
                                         Controller::MOVE_RIGHT | Controller::MOVE_LEFT)) > 0) {
            skinAnim.playAnimation("Running");
        } /* else if (character.ctrl->movement & (Controller::MOVE_LEFT)) {
             skinAnim.playAnimation("StrafingLeft");
         } else if (character.ctrl->movement & (Controller::MOVE_RIGHT)) {
             skinAnim.playAnimation("StrafingRight");
         } */
        else {
            skinAnim.playAnimation("Idle");
        }
        break;
    case Controller::STATE_WALL_HANG:
        skinAnim.playAnimation("WallHang");
        break;
    case Controller::STATE_WALL_JUMP:
        skinAnim.playAnimation("Jumping");
        break;
    }
}

void Platformer_cameraCollision(Camera &camera) {
    camera.targetView.distance = glm::max(6.0f - camera.currentView.pitch * 0.06f, 0.1f);

    float tmin = 0.0f;
    float tmax = 0.0f;

    glm::vec3 targetOrientation =
        glm::angleAxis(glm::radians(camera.targetView.yaw), primer::UP) *
        glm::angleAxis(glm::radians(camera.targetView.pitch), primer::RIGHT) * primer::FORWARD;

    ecs::System<CPawn, CCollider>::for_each([&](ecs::Entity *e, Pawn &pawn, Collider &collider) {
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

void Platformer_cameraFollow(Camera &camera, Character &character, float dt) {
    float f = character.actor->velocity.x * character.actor->velocity.x +
              character.actor->velocity.z * character.actor->velocity.z;
    glm::vec3 e = character.node->euler.data();
    float offs = 0.0f;
    if (character.ctrl->movement & Controller::MOVE_LEFT) {
        offs = 90.0f;
    } else if (character.ctrl->movement & Controller::MOVE_RIGHT) {
        offs = -90.0f;
    }
    character.node->euler = {
        0.0f,
        glm::mix(e.y, camera.targetView.yaw + offs, glm::min(f, 1.0f) * 12.0f * dt),
        0.0f,
    };
    /*character.node->model();
    if (!primer::isNear(primer::UP, character.ctrl->up)) {
        glm::vec3 axis = glm::normalize(glm::cross(primer::UP, character.ctrl->up));
        float angle = glm::acos(glm::dot(primer::UP, character.ctrl->up));
        character.node->rotation *= glm::angleAxis(glm::radians(angle), axis);
    }*/
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
    // glm::vec3 R = glm::reflect(actor.velocity, collisionNormal);
    // actor.velocity = actor.restitution * R;
    actor.velocity.y = -actor.velocity.y * actor.restitution;
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

bool Platformer_keyDown(Character &player, int key, int mods) {
    switch (key) {
    case SDLK_w:
        player.ctrl->movement |= Controller::MOVE_FORWARD;
        break;
    case SDLK_a:
        player.ctrl->movement |= Controller::MOVE_LEFT;
        break;
    case SDLK_s:
        player.ctrl->movement |= Controller::MOVE_BACKWARD;
        break;
    case SDLK_d:
        player.ctrl->movement |= Controller::MOVE_RIGHT;
        break;
    case SDLK_SPACE:
        player.ctrl->movement |= Controller::MOVE_JUMP;
        break;
    default:
        return false;
    }
    return true;
}

bool Platformer_keyUp(Character &player, int key, int mods) {
    switch (key) {
    case SDLK_w:
        player.ctrl->movement &= ~Controller::MOVE_FORWARD;
        break;
    case SDLK_a:
        player.ctrl->movement &= ~Controller::MOVE_LEFT;
        break;
    case SDLK_s:
        player.ctrl->movement &= ~Controller::MOVE_BACKWARD;
        break;
    case SDLK_d:
        player.ctrl->movement &= ~Controller::MOVE_RIGHT;
        break;
    case SDLK_SPACE:
        player.ctrl->movement &= ~Controller::MOVE_JUMP;
        break;
    default:
        return false;
    }
    return true;
}