#pragma once

enum JoinStatus
{
  JOIN_STATUS_FAILED_TOO_MANY_PLAYERS,
  JOIN_STATUS_SUCCESS
};

// TODO: Add in a clientVersion to verify API.
typedef struct JoinRequestMessage
{
  GUID playerId;
} JoinRequestMessage;

typedef struct JoinResponseMessage
{
  enum JoinStatus code;
} JoinResponseMessage;
