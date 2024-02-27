#pragma once

#include "Otter/GameState/Player/PlayerInput.h"
#include "Otter/Networking/Messages/Message.h"
#include "Otter/Networking/export.h"

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

OTTER_API Message* message_create_player_move(
    GUID* id, GUID* playerId, PlayerInput direction, uint32_t tickId);
OTTER_API Message* message_create_player_position(GUID* id, uint32_t tickId);
