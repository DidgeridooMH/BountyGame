#include "Otter/Networking/Messages/EntityMessages.h"

Message* message_create_player_move(
    GUID* id, GUID* playerId, PlayerInput direction, uint32_t tickId)
{
  Message* message =
      message_create(MT_PLAYER_MOVE, id, sizeof(PlayerMoveMessage), tickId);
  if (message == NULL)
  {
    return NULL;
  }

  PlayerMoveMessage* payload = message->payload;
  payload->playerId          = *playerId;
  payload->direction         = direction;

  return message;
}

Message* message_create_player_attributes(GUID* id, uint32_t tickId)
{
  Message* message = message_create(
      MT_PLAYER_ATTRIBUTES, id, sizeof(PlayerAttributesMessage), tickId);
  if (message == NULL)
  {
    return NULL;
  }

  PlayerAttributesMessage* payload = message->payload;
  memset(payload, 0, sizeof(PlayerAttributesMessage));

  return message;
}

Message* message_create_player_left(GUID* id, GUID* playerId, uint32_t tickId)
{
  Message* message =
      message_create(MT_PLAYER_LEFT, id, sizeof(PlayerLeftMessage), tickId);
  if (message == NULL)
  {
    return NULL;
  }

  PlayerLeftMessage* payload = message->payload;
  payload->playerId          = *playerId;

  return message;
}
