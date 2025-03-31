#pragma once

#include "window.h"
#include <cstdint>

static constexpr uint8_t MOVE_FORWARD = 0b00000001;
static constexpr uint8_t MOVE_LEFT = 0b00000010;
static constexpr uint8_t MOVE_BACKWARD = 0b00000100;
static constexpr uint8_t MOVE_RIGHT = 0b00001000;
static constexpr uint8_t MOVE_JUMP = 0b00010000;

bool PlayerInput_keyDown(uint8_t &movement, int key, int mods);

bool PlayerInput_keyUp(uint8_t &movement, int key, int mods);