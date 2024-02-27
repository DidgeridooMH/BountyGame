#include "Otter/Networking/Messages/ControlMessages.h"

Message* message_create_join_request(GUID* id)
{
  return message_create(MT_JOIN_REQUEST, id, 0, 0);
}

Message* message_create_join_response(
    GUID* id, enum JoinStatus status, uint32_t tickId)
{
  Message* message =
      message_create(MT_JOIN_RESPONSE, id, sizeof(JoinResponseMessage), tickId);
  if (message == NULL)
  {
    return NULL;
  }

  ((JoinResponseMessage*) message->payload)->status = status;

  return message;
}

Message* message_create_heartbeat(GUID* id, uint32_t tickId)
{
  return message_create(MT_HEARTBEAT, id, 0, tickId);
}

Message* message_create_leave_request(GUID* id)
{
  return message_create(MT_LEAVE_REQUEST, id, 0, 0);
}
