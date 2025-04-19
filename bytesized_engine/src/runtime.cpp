#include "runtime.h"
#include "component.h"

void Runtime::init(RuntimeOptions options) {
    _options = options;
    events.clear();
}

void Runtime::update(Character &player, float dt) {
    if (_options & RUNTIME_INTERACTABLES) {
        ecs::System<CPawn, CInteractable>::for_each(
            [this, dt, &player](ecs::Entity *e, Pawn &pawn, Interactable &interactable) {
                (void)e;
                glm::vec3 d = player.actor->trs->t() - pawn.trs->position();
                if (glm::dot(d, d) < interactable.radii * interactable.radii) {
                    interactable.inRadius(dt, (gpu::Node *)pawn.trs, player.node);
                    if (_interacting) {
                        interactable.interaction(dt, (gpu::Node *)pawn.trs, player.node);
                    }
                }
            });
        _interacting = false;
    }
    if (_options & RUNTIME_EVENTS) {
        if (!events.empty()) {
            auto &event = events.front();
            event.update(&event, dt);
            if (event.timer.elapsed()) {
                event.final(&event, dt);
                events.pop_front();
            }
        }
    }
}

void Runtime::interact() { _interacting = true; }