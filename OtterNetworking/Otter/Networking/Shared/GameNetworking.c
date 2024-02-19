#include "GameNetworking.h"

#include "Otter/Networking/Messages/ControlMessages.h"
#include "Otter/Networking/Messages/EntityMessages.h"

#define FIXED_RECV_MESSAGE(type)                                          \
  {                                                                       \
    message->payload = malloc(sizeof(type));                              \
    if (message->payload == NULL)                                         \
    {                                                                     \
      fprintf(stderr, "Out of memory\n");                                 \
      free(message);                                                      \
      return NULL;                                                        \
    }                                                                     \
    if (message->header.payloadSize != sizeof(type))                      \
    {                                                                     \
      fprintf(stderr, "Size mismatch. Possible API version mismatch.\n"); \
      free(message->payload);                                             \
      free(message);                                                      \
      return NULL;                                                        \
    }                                                                     \
  }

static bool game_networking_recv(SOCKET socket, char* buffer, int bytesToRead)
{
  int bytesRemaining = bytesToRead;
  while (bytesRemaining > 0)
  {
    int bytesReturned = recv(socket,
        (char*) buffer + (bytesToRead - bytesRemaining), bytesRemaining, 0);
    if (bytesReturned > 0)
    {
      bytesRemaining -= bytesReturned;
    }
    else if (bytesReturned == 0)
    {
      fprintf(
          stderr, "Could not receive message because client disconnected\n");
      return false;
    }
    else if (WSAGetLastError() != WSAEWOULDBLOCK)
    {
      fprintf(stderr, "(E%d) Could not receive message.\n", WSAGetLastError());
      return false;
    }
  }
  return true;
}

Message* game_networking_recv_message(SOCKET socket)
{
  Message* message = malloc(sizeof(Message));
  if (message == NULL)
  {
    fprintf(stderr, "Out of memory\n");
    return NULL;
  }

  if (!game_networking_recv(
          socket, (char*) &message->header, sizeof(MessageHeader)))
  {
    free(message);
    return NULL;
  }

  int payloadSize = 0;
  switch (message->header.type)
  {
  case MT_JOIN_REQUEST:
    FIXED_RECV_MESSAGE(JoinRequestMessage)
    break;
  case MT_JOIN_RESPONSE:
    FIXED_RECV_MESSAGE(JoinResponseMessage)
    break;
  case MT_PLAYER_MOVE:
    FIXED_RECV_MESSAGE(PlayerMoveMessage)
    break;
  case MT_PLAYER_POSITION:
    FIXED_RECV_MESSAGE(PlayerPositionMessage)
    break;
  default:
    fprintf(stderr, "Unknown message type\n");
    free(message);
    return NULL;
  }

  if (!game_networking_recv(
          socket, (char*) message->payload, message->header.payloadSize))
  {
    fprintf(stderr, "Unable to receive payload of message.\n");
    free(message->payload);
    free(message);
    message = NULL;
  }

  return message;
}

static void game_networking_send(SOCKET socket, void* buffer, int size)
{
  int cursor = 0;
  while (cursor < size)
  {
    int bytesSent = send(socket, (char*) buffer + cursor, size - cursor, 0);
    if (bytesSent == SOCKET_ERROR)
    {
      fprintf(stderr, "(E%d) Unable to send packet of size %d.\n",
          WSAGetLastError(), size);
      break;
    }
    cursor += bytesSent;
  }
}

void game_networking_send_message(SOCKET socket, Message* message)
{
  game_networking_send(socket, &message->header, sizeof(MessageHeader));
  game_networking_send(socket, message->payload, message->header.payloadSize);
}
