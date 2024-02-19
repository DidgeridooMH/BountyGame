#pragma once

#include "Otter/Networking/export.h"

enum MessageType
{
  MT_JOIN_REQUEST,
  MT_JOIN_RESPONSE,
  MT_PLAYER_MOVE,
  MT_PLAYER_POSITION
};

typedef struct MessageHeader
{
  GUID entity;
  enum MessageType type;
  int payloadSize;
} MessageHeader;

typedef struct Message
{
  MessageHeader header;
  void* payload;
} Message;
