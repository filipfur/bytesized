#pragma once

#include "camera.h"
#include "character.h"

void Platformer_updateActorGravity(float dt);
void Platformer_updateActorMovement(float dt);
void Platformer_updatePlatformCollisions(float dt);
void Platformer_updateKineticCollisions(float dt);
void Platformer_updatePlayerController(Character &character, float dt);
void Platformer_setPlayerAnimation(Character &character);
void Platformer_cameraCollision(Camera &camera);
void Platformer_cameraFollow(Camera &camera, Character &character, float dt);

bool Platformer_keyDown(Character &player, int key, int mods);
bool Platformer_keyUp(Character &player, int key, int mods);