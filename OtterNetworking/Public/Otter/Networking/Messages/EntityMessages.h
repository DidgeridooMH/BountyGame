#pragma once

#include "Otter/GameState/Player/PlayerInput.h"
#include "Otter/Networking/Messages/Message.h"
#include "Otter/Networking/export.h"

typedef struct PlayerMoveMessage
{
  GUID playerId;
  PlayerInput direction;
} PlayerMoveMessage;

typedef struct PlayerAttributesMessage
{
  GUID playerId;
  float positionX;
  float positionY;
  float velocityX;
  float velocityY;
} PlayerAttributesMessage;

typedef struct PlayerLeftMessage
{
  GUID playerId;
} PlayerLeftMessage;

OTTERNETWORKING_API Message* message_create_player_move(
    GUID* id, GUID* playerId, PlayerInput direction, uint32_t tickId);
OTTERNETWORKING_API Message* message_create_player_attributes(
    GUID* id, uint32_t tickId);
OTTERNETWORKING_API Message* message_create_player_left(
    GUID* id, GUID* playerId, uint32_t tickId);
