#pragma once

#include "Otter/Networking/Messages/Message.h"
#include "Otter/Networking/export.h"

enum JoinStatus
{
  JOIN_STATUS_FAILED_TOO_MANY_PLAYERS,
  JOIN_STATUS_SUCCESS
};

typedef struct JoinResponseMessage
{
  enum JoinStatus status;
} JoinResponseMessage;

OTTER_API Message* message_create_join_request(GUID* id);
OTTER_API Message* message_create_join_response(
    GUID* id, enum JoinStatus status);
OTTER_API Message* message_create_heartbeat(GUID* id);
