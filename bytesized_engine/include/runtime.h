#pragma once

#include "character.h"
#include "timer.h"
#include <functional>
#include <list>

enum RuntimeOptions {
    RUNTIME_NONE = 0,
    RUNTIME_EVENTS = (1 << 0),
    RUNTIME_INTERACTABLES = (1 << 1),
    RUNTIME_ALL = 0xFFFF
};

struct Runtime {
    struct Event {
        std::function<void(Event *, float)> update;
        std::function<void(Event *, float)> final;
        Timer timer;
    };

    void init(RuntimeOptions options = RUNTIME_ALL);

    void update(Character &player, float dt);

    void interact();

  private:
    RuntimeOptions _options;
    bool _interacting{false};

  public:
    std::list<Event> events;
};