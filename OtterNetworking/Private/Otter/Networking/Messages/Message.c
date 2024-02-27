#include "Otter/Networking/Messages/Message.h"

Message* message_create(
    enum MessageType type, GUID* guid, int payloadSize, uint32_t tickId)
{
  Message* message =
      malloc(sizeof(Message) + sizeof(MessageHeader) + payloadSize);
  if (message == NULL)
  {
    return NULL;
  }

  message->header = (MessageHeader*) ((char*) message + sizeof(Message));
  message->header->entity      = *guid;
  message->header->payloadSize = payloadSize;
  message->header->type        = type;
  message->header->tickId      = tickId;

  if (payloadSize > 0)
  {
    message->payload = (char*) message->header + sizeof(MessageHeader);
  }
  else
  {
    message->payload = NULL;
  }

  return message;
}

void message_destroy(Message* message)
{
  free(message);
}
