#include "Otter/Networking/Shared/GameNetworking.h"

#include "Otter/Networking/Messages/ControlMessages.h"
#include "Otter/Networking/Messages/EntityMessages.h"

static bool game_networking_recv_packet(
    SOCKET socket, char* buffer, int bytesToRead)
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

static bool game_networking_recv_datagram(
    SOCKET socket, struct sockaddr_in* peerAddress, char* buffer, int* size)
{
  int peerAddressLength = sizeof(struct sockaddr_in);

  int bytesRead = recvfrom(socket, buffer, *size, 0,
      (struct sockaddr*) peerAddress, &peerAddressLength);
  if (bytesRead > 0)
  {
    *size = bytesRead;
    return true;
  }

  return false;
}

Message* game_networking_recv_message_tcp(SOCKET socket)
{
  Message* message = malloc(sizeof(Message));
  if (message == NULL)
  {
    fprintf(stderr, "Out of memory\n");
    return NULL;
  }

  if (!game_networking_recv_packet(
          socket, (char*) &message->header, sizeof(MessageHeader)))
  {
    free(message);
    return NULL;
  }

  message->payload = malloc(message->header.payloadSize);
  if (message->payload == NULL)
  {
    fprintf(stderr, "Out of memory\n");
    free(message);
    return NULL;
  }

  if (!game_networking_recv_packet(
          socket, (char*) message->payload, message->header.payloadSize))
  {
    fprintf(stderr, "Unable to receive payload of message.\n");
    free(message->payload);
    free(message);
    message = NULL;
  }

  return message;
}

Message* game_networking_recv_message_udp(
    SOCKET socket, struct sockaddr_in* address)
{
  char buffer[2048] = {0};
  int bufferLength  = sizeof(buffer);
  if (game_networking_recv_datagram(socket, address, buffer, &bufferLength))
  {
    Message* message = malloc(sizeof(Message));
    if (message == NULL)
    {
      fprintf(stderr, "Out of memory");
      return NULL;
    }
    memcpy(&message->header, buffer, sizeof(MessageHeader));

    if (bufferLength != message->header.payloadSize + sizeof(MessageHeader))
    {
      fprintf(stderr, "Datagram did not contain a valid message.");
      free(message);
      return NULL;
    }

    void* payload = malloc(message->header.payloadSize);
    if (payload == NULL)
    {
      fprintf(stderr, "Out of memory");
      return NULL;
    }
    memcpy(
        payload, buffer + sizeof(MessageHeader), message->header.payloadSize);

    message->payload = payload;

    return message;
  }
  return NULL;
}

static void game_networking_send_tcp(
    SOCKET socket, const void* buffer, int size)
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

static void game_networking_send_udp(SOCKET socket,
    const struct sockaddr_in* target, const void* buffer, int size)
{
  sendto(socket, buffer, size, 0, (struct sockaddr*) target,
      sizeof(struct sockaddr));
}

void game_networking_send_message_tcp(SOCKET socket, const Message* message)
{
  game_networking_send_tcp(socket, &message->header, sizeof(MessageHeader));
  if (message->payload != NULL)
  {
    game_networking_send_tcp(
        socket, message->payload, message->header.payloadSize);
  }
}

void game_networking_send_message_udp(
    SOCKET socket, const struct sockaddr_in* target, const Message* message)
{
  char buffer[2048] = {0};
  memcpy(buffer, &message->header, sizeof(MessageHeader));
  if (message->payload != NULL)
  {
    memcpy(buffer + sizeof(MessageHeader), message->payload,
        message->header.payloadSize);
  }
  game_networking_send_udp(socket, target, buffer,
      sizeof(MessageHeader) + message->header.payloadSize);
}
