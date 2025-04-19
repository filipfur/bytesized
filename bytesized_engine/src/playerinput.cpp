#include "playerinput.h"

bool PlayerInput_keyDown(uint8_t &movement, int key, int /*mods*/) {
    switch (key) {
    case SDLK_w:
        movement |= MOVE_FORWARD;
        break;
    case SDLK_a:
        movement |= MOVE_LEFT;
        break;
    case SDLK_s:
        movement |= MOVE_BACKWARD;
        break;
    case SDLK_d:
        movement |= MOVE_RIGHT;
        break;
    case SDLK_SPACE:
        movement |= MOVE_JUMP;
        break;
    default:
        return false;
    }
    return true;
}

bool PlayerInput_keyUp(uint8_t &movement, int key, int /*mods*/) {
    switch (key) {
    case SDLK_w:
        movement &= ~MOVE_FORWARD;
        break;
    case SDLK_a:
        movement &= ~MOVE_LEFT;
        break;
    case SDLK_s:
        movement &= ~MOVE_BACKWARD;
        break;
    case SDLK_d:
        movement &= ~MOVE_RIGHT;
        break;
    case SDLK_SPACE:
        movement &= ~MOVE_JUMP;
        break;
    default:
        return false;
    }
    return true;
}