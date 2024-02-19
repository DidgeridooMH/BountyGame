#include "Otter/Networking/Messages/ControlMessages.h"

Message* message_create_join_request(GUID* id)
{
  return message_create(MT_JOIN_REQUEST, id, 0);
}

Message* message_create_join_response(GUID* id, enum JoinStatus status)
{
  Message* message =
      message_create(MT_JOIN_RESPONSE, id, sizeof(JoinResponseMessage));
  if (message == NULL)
  {
    return NULL;
  }

  message->payload = malloc(sizeof(JoinResponseMessage));
  if (message->payload == NULL)
  {
    free(message);
    return NULL;
  }
  ((JoinResponseMessage*) message->payload)->status = status;

  return message;
}

Message* message_create_heartbeat(GUID* id)
{
  return message_create(MT_HEARTBEAT, id, 0);
}
