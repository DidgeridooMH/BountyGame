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
  GUID clientId;
} JoinResponseMessage;

OTTERNETWORKING_API Message* message_create_join_request();
OTTERNETWORKING_API Message* message_create_join_response(
    GUID* id, enum JoinStatus status, GUID* clientId, uint32_t tickId);
OTTERNETWORKING_API Message* message_create_heartbeat(
    GUID* id, uint32_t tickId);
OTTERNETWORKING_API Message* message_create_leave_request(GUID* id);
