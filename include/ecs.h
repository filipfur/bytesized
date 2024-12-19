#pragma once

#include "recycler.hpp"
#include <cstdint>
#include <functional>

#define ECS_MAX_ENTITIES 100
#define ECS_MAX_COMPONENTS 32

namespace ecs {

using Entity = uint32_t;

Entity *create();
void free(Entity *entity);

uint32_t ID(const Entity *entity);

recycler<Entity, ECS_MAX_ENTITIES> &entities();

extern uint32_t _nextComponentNumber;

static inline bool has(const Entity *entity, uint32_t mask) { return (*entity & mask) == mask; }

template <class T, uint32_t Variant = 0, bool Static = false> class Component {
  public:
    using value_type = T;

    Component() {}

    static void attach(Entity *entity) { *entity |= bit_mask(); }

    template <typename... Args> static void attach(Entity *entity, Args... args) {
        attach(entity);
        attach(args...);
    }

    static bool has(const Entity *entity) { return (*entity & bit_mask()) != 0; }

    static void detach(Entity *entity) { *entity &= ~bit_mask(); }

    static T &get(const Entity *entity) { return Static ? _ts[0] : _ts[ID(entity)]; }

    static void set(const T &t, const Entity *entity) {
        if (Static) {
            _ts[0] = t;
        } else {
            _ts[ID(entity)] = t;
        }
    }

    static uint32_t bit_mask() { return (1 << _id); }

  private:
    static uint32_t _id;
    static T _ts[];
};

template <typename... T> void attach(Entity *entity) {
    (T::attach(entity), ...); // fold expression
}

template <class T, uint32_t Variant, bool Static>
T Component<T, Variant, Static>::_ts[Static ? 1 : ECS_MAX_ENTITIES] = {};

template <class T, uint32_t Variant, bool Static>
uint32_t Component<T, Variant, Static>::_id{_nextComponentNumber++};

template <class... T> class System {
  public:
    static void for_each(std::function<void(ecs::Entity *, typename T::value_type &...)> callback) {
        auto mask = System<T...>::mask();
        auto &entities = ecs::entities();
        for (size_t i{0}; i < entities.count(); ++i) {
            Entity *entity = entities.data() + i;
            if (ecs::has(entity, mask)) {
                (callback(entity, T::get(entity)...));
            }
        }
    }

    /// @brief ordered means permuations, unordered means combinations
    static void combine(std::function<void(ecs::Entity *, typename T::value_type &...,
                                           ecs::Entity *, typename T::value_type &...)>
                            callback,
                        bool ordered) {
        auto mask = System<T...>::mask();
        auto &entities = ecs::entities();
        for (size_t i{0}; i < entities.count(); ++i) {
            Entity *entity = entities.data() + i;
            if (ecs::has(entity, mask)) {
                for (size_t j{0}; j < entities.count(); ++j) {
                    if (ordered) {
                        if (i == j) {
                            continue;
                        }
                    } else {
                        if (i >= j) {
                            continue;
                        }
                    }
                    Entity *other = entities.data() + j;
                    if (ecs::has(other, mask)) {
                        (callback(entity, T::get(entity)..., other, T::get(other)...));
                    }
                }
            }
        }
    }

    static uint32_t mask() { return (T::bit_mask() + ...); }
};

} // namespace ecs
