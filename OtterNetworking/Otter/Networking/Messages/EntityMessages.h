#pragma once

#include "Otter/GameState/Player/PlayerInput.h"

typedef struct PlayerMoveMessage
{
  GUID playerId;
  PlayerInput direction;
} PlayerMoveMessage;

typedef struct PlayerPositionMessage
{
  GUID playerId;
  float x;
  float y;
} PlayerPositionMessage;
