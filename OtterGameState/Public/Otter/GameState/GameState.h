#pragma once

#include "Otter/GameState/export.h"
#include "Otter/GameState/Player/PlayerInput.h"

OTTER_API void game_state_update(PlayerInput* playerInputs, float deltaTime);
