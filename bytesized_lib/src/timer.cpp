#include "timer.h"

Timer::Timer() : _duration{}, _active{false}, _expired{} {}

Timer::Timer(Time::Raw duration)
    : _duration{duration}, _active{true}, _expired{Time::raw() + duration} {}

bool Timer::active() const { return _active; }

Time::Raw Timer::timeLeft() const { return _expired - Time::raw(); }

float Timer::progress() const {
    float x = 1.0f - static_cast<float>(timeLeft()) / static_cast<float>(_duration);
    return x < 0.0f ? 0.0f : x;
}

bool Timer::elapsed() const { return _active && Time::raw() >= _expired; }

bool Timer::cancel() {
    if (_active) {
        _active = false;
        return true;
    }
    return false;
}

void Timer::reset() {
    _active = true;
    _expired = Time::millseconds() + _duration;
}

void Timer::set(Time::Raw duration) {
    _duration = duration;
    _active = true;
    _expired = Time::millseconds() + _duration;
}