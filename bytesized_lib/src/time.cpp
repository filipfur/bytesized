#include "systime.h"

static Time::Raw _time{};

float Time::seconds() { return _time * 1e-3f; }

Time::Raw Time::raw() { return _time; }

Time::Raw Time::millseconds() { return _time; }

void Time::increment(Time::Raw delta_ms) { _time += delta_ms; }