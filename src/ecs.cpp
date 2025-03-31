#include "ecs.h"

static recycler<ecs::Entity, ECS_MAX_ENTITIES> _entities = {};
recycler<ecs::Entity, ECS_MAX_ENTITIES> &ecs::entities() { return _entities; }

ecs::Entity *ecs::create_entity() { return _entities.acquire(); }
void ecs::dispose_entity(ecs::Entity *entity) {
    *entity = 0;
    _entities.free(entity);
}
void ecs::free(ecs::Entity *entity) { _entities.free(entity); }

uint32_t ecs::ID(const Entity *entity) { return (entity - _entities.data()); }