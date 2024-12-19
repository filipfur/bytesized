#include "ecs.h"

uint32_t ecs::_nextComponentNumber{0};
static recycler<ecs::Entity, ECS_MAX_ENTITIES> _entities = {};
recycler<ecs::Entity, ECS_MAX_ENTITIES> &ecs::entities() { return _entities; }

ecs::Entity *ecs::create() { return _entities.acquire(); }
void ecs::free(ecs::Entity *entity) { _entities.free(entity); }

uint32_t ecs::ID(const Entity *entity) { return (entity - _entities.data()); }