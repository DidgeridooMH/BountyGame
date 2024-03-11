#pragma once

#include "Otter/GameState/Player/PlayerInput.h"
#include "Otter/GameState/export.h"

OTTERGAMESTATE_API void game_state_update(
    PlayerInput* playerInputs, float deltaTime);
