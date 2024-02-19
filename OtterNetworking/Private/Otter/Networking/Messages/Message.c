#include "Otter/Networking/Messages/Message.h"

Message* message_create(enum MessageType type, GUID* guid, int payloadSize)
{
  Message* message = malloc(sizeof(Message));
  if (message == NULL)
  {
    return NULL;
  }

  message->header.entity      = *guid;
  message->header.payloadSize = payloadSize;
  message->header.type        = type;

  message->payload = NULL;

  return message;
}

void message_destroy(Message* message)
{
  free(message->payload);
  free(message);
}
