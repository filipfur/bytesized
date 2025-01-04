#include "component.h"
#include "ecs.h"
#include "geom_collision.h"
#include "primer.h"

static void _collisionResponse(Actor &actor, const glm::vec3 &collisionNormal,
                               float penetrationDepth) {
    actor.trs->translation += collisionNormal * penetrationDepth * 1.01f;
    glm::vec3 R = glm::reflect(actor.velocity * 0.5f, collisionNormal);
    actor.velocity = glm::dot(R, R) > 0.1f ? R : glm::vec3{0.0f, 0.0f, 0.0f};
}

static geom::Collision _collision;

static bool _handleCollision(Actor &actor_a, Collider &collider_a, TRS *trs_b, Collider &collider_b,
                             Actor *actor_b) {
    collider_a.geometry->trs = collider_a.transforming ? actor_a.trs : nullptr;
    collider_b.geometry->trs = collider_b.transforming ? trs_b : nullptr;
    if (geom::collides(*collider_a.geometry, *collider_b.geometry, &_collision)) {
        // print_glm(collision.normal);
        // printf("penetrationDepth=%f\n", collision.penetrationDepth);
        _collisionResponse(actor_a, _collision.normal, _collision.penetrationDepth);
        if (actor_b) {
            _collisionResponse(actor_a, -_collision.normal, _collision.penetrationDepth);
        }
        return true;
    }
    return false;
}

void physics_step(float dt) {
    ecs::System<CController>::for_each([dt](ecs::Entity *entity, Controller &controller) {
        if (controller.onGround) {
            Actor &actor = CActor::get(entity);
            bool moving = glm::dot(controller.walkControl, controller.walkControl) > 0.1f;
            if (moving) {
                actor.velocity += (actor.trs->r() * glm::normalize(controller.walkControl)) *
                                  controller.walkSpeed * dt;
            }
            if (controller.jumpControl == 1) {
                actor.velocity.y += controller.jumpSpeed;
                controller.jumpControl = 2;
            } else if (controller.jumpControl == 3) {
                actor.velocity.y += controller.jumpSpeed;
                controller.jumpControl = 4;
            }
            /*if (moving || jumped) {
                if (CPawn::has(entity)) {
                    CPawn::detach(entity);
                    CActor::attach(entity);
                }
            }*/
        }
    });
    ecs::System<CActor>::for_each([dt](ecs::Entity *entity, Actor &actor) {
        // gravity
        bool applyGravity = true;
        if (actor.velocity.y * actor.velocity.y < 0.001f) {
            if (Controller *ctrl = CController::get_pointer(entity)) {
                Collider &collider_a = CCollider::get(entity);
                ctrl->onGround = ecs::System<CPawn, CCollider>::until(
                    [&](ecs::Entity *entity_b, Pawn &pawn_b, Collider &collider_b) {
                        ctrl->floorSense->trs = collider_a.transforming ? actor.trs : nullptr;
                        collider_b.geometry->trs = collider_b.transforming ? pawn_b.trs : nullptr;
                        // TODO: Rotate up vector according to trs of actor
                        if (geom::collides(*ctrl->floorSense, *collider_b.geometry, &_collision) &&
                            glm::dot(_collision.normal, glm::vec3{0.0f, 1.0f, 0.0f}) > 0.5f) {
                            return true;
                        }
                        return false;
                    });
                applyGravity = !ctrl->onGround;
            }
        }
        if (applyGravity) {
            static const float maxFallSpeed{100'000.0f};
            if (actor.velocity.y > -maxFallSpeed) {
                actor.velocity.y -= 9.81f * dt;
            }
        }
        // apply velocity
        actor.trs->translation += actor.velocity * dt;
    });
    ecs::System<CActor, CCollider>::for_each(
        [](ecs::Entity *entity_a, Actor &actor_a, Collider &collider_a) {
            ecs::System<CPawn, CCollider>::until(
                [&](ecs::Entity *entity_b, Pawn &pawn_b, Collider &collider_b) {
                    if (_handleCollision(actor_a, collider_a, pawn_b.trs, collider_b, nullptr)) {
                        if (CController::has(entity_a)) {
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
    ecs::System<CActor, CCollider>::combine(
        [](ecs::Entity *entity_a, Actor &actor_a, Collider &collider_a, ecs::Entity *entity_b,
           Actor &actor_b, Collider &collider_b) {
            if (_handleCollision(actor_a, collider_a, actor_b.trs, collider_b, &actor_b)) {
                /*CActor::detach(entity_a);
                CActor::detach(entity_b);*/
            }
        },
        false);
}