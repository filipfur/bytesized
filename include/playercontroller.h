#pragma once

#include "camera.h"
#include "character.h"

void PlayerController_updateActorGravity(float dt);
void PlayerController_updateActorMovement(float dt);
void PlayerController_updatePlatformCollisions();
void PlayerController_updateKineticCollisions();
void PlayerController_firstPersonPlayerController(Character &character, float dt);
void PlayerController_thirdPersonPlayerController(Character &character, float dt);
void PlayerController_setPlayerAnimation(Character &character);
void PlayerController_cameraCollision(Camera &camera);
void PlayerController_firstPersonCameraFollow(Camera &camera, Character &character, float dt);
void PlayerController_thirdPersonCameraFollow(Camera &camera, Character &character, float dt);
