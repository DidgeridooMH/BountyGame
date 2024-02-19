#include "GameClient.h"

#include "Otter/Networking/Shared/GameNetworking.h"

// TODO: Fix this to be a client thingy.
bool game_client_connect(GameClient* client, const char* host, const char* port)
{
  struct addrinfo serverAddress = {.ai_family = AF_INET,
      .ai_socktype                            = SOCK_STREAM,
      .ai_protocol                            = IPPROTO_TCP,
      .ai_flags                               = AI_PASSIVE};

  struct addrinfo* resolvedAddress;
  if (getaddrinfo(host, port, &serverAddress, &resolvedAddress))
  {
    fprintf(stderr, "Unable to resolve address %s:%s because %d", host, port,
        GetLastError());
    return false;
  }

  client->socket = socket(resolvedAddress->ai_family,
      resolvedAddress->ai_socktype, resolvedAddress->ai_protocol);

  if (client->socket == INVALID_SOCKET)
  {
    fprintf(stderr, "(%d) Unable to open socket.", GetLastError());
    freeaddrinfo(resolvedAddress);
    return false;
  }

  if (connect(client->socket, resolvedAddress->ai_addr,
          (int) resolvedAddress->ai_addrlen)
      == SOCKET_ERROR)
  {
    fprintf(stderr, "(%d) Unable to connect to server.", GetLastError());
    freeaddrinfo(resolvedAddress);
    return false;
  }

  uint32_t mode = 1;
  ioctlsocket(client->socket, FIONBIO, &mode);

  freeaddrinfo(resolvedAddress);

  return true;
}

Message* game_client_get_message(GameClient* client)
{
  return game_networking_recv_message(client->socket);
}

void game_client_send_message(GameClient* client, Message* message)
{
  game_networking_send_message(client->socket, message);
}

void game_client_destroy(GameClient* client)
{
  shutdown(client->socket, SD_BOTH);
  closesocket(client->socket);
}
