#pragma once

#include "recycler.hpp"
#include <cstdint>
#include <functional>

#define ECS_MAX_ENTITIES 100
#define ECS_MAX_COMPONENTS 32

namespace ecs {

using Entity = uint32_t;

Entity *create_entity();
void free(Entity *entity);

uint32_t ID(const Entity *entity);

recycler<Entity, ECS_MAX_ENTITIES> &entities();

static inline bool has(const Entity *entity, uint32_t mask) { return (*entity & mask) == mask; }

template <class T, uint32_t Id, bool Static = false> class Component {
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
    static T *get_pointer(const Entity *entity) {
        if (Static) {
            return &_ts[0];
        }
        return has(entity) ? &_ts[ID(entity)] : nullptr;
    }

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

template <class T, uint32_t Id, bool Static>
T Component<T, Id, Static>::_ts[Static ? 1 : ECS_MAX_ENTITIES] = {};

template <class T, uint32_t Id, bool Static> uint32_t Component<T, Id, Static>::_id{Id};

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

    /// @brief for every until condition met
    static bool until(std::function<bool(ecs::Entity *, typename T::value_type &...)> callback) {
        auto mask = System<T...>::mask();
        auto &entities = ecs::entities();
        for (size_t i{0}; i < entities.count(); ++i) {
            Entity *entity = entities.data() + i;
            if (ecs::has(entity, mask)) {
                if (callback(entity, T::get(entity)...)) {
                    return true;
                }
            }
        }
        return false;
    }

    static ecs::Entity *find(std::function<bool(typename T::value_type &...)> callback) {
        auto mask = System<T...>::mask();
        auto &entities = ecs::entities();
        for (size_t i{0}; i < entities.count(); ++i) {
            Entity *entity = entities.data() + i;
            if (ecs::has(entity, mask)) {
                if (callback(T::get(entity)...)) {
                    return entity;
                }
            }
        }
        return nullptr;
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
