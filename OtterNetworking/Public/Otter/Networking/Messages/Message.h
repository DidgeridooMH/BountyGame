#pragma once

#include "Otter/Networking/export.h"

enum MessageType
{
  MT_JOIN_REQUEST,
  MT_JOIN_RESPONSE,
  MT_LEAVE_REQUEST,
  MT_PLAYER_MOVE,
  MT_PLAYER_ATTRIBUTES,
  MT_PLAYER_LEFT,
  MT_HEARTBEAT
};

typedef struct MessageHeader
{
  GUID entity;
  enum MessageType type;
  uint32_t tickId;
  int payloadSize;
} MessageHeader;

typedef struct Message
{
  MessageHeader* header;
  void* payload;
} Message;

OTTERNETWORKING_API Message* message_create(
    enum MessageType type, GUID* guid, int payloadSize, uint32_t tickId);
OTTERNETWORKING_API void message_destroy(Message* message);
