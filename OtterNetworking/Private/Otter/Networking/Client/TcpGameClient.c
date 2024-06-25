#include "Otter/Networking/Client/TcpGameClient.h"

#include "Otter/Networking/Shared/GameNetworking.h"

bool tcp_game_client_connect(
    TcpGameClient* client, const char* host, const char* port)
{
  struct addrinfo serverAddress = {.ai_family = AF_INET,
      .ai_socktype                            = SOCK_STREAM,
      .ai_protocol                            = IPPROTO_TCP,
      .ai_flags                               = AI_PASSIVE};

  struct addrinfo* resolvedAddress;
  if (getaddrinfo(host, port, &serverAddress, &resolvedAddress))
  {
    fprintf(stderr, "Unable to resolve address %s:%s because %lu", host, port,
        GetLastError());
    return false;
  }

  client->socket = socket(resolvedAddress->ai_family,
      resolvedAddress->ai_socktype, resolvedAddress->ai_protocol);

  if (client->socket == INVALID_SOCKET)
  {
    fprintf(stderr, "(%lu) Unable to open socket.", GetLastError());
    freeaddrinfo(resolvedAddress);
    return false;
  }

  if (connect(client->socket, resolvedAddress->ai_addr,
          (int) resolvedAddress->ai_addrlen)
      == SOCKET_ERROR)
  {
    fprintf(stderr, "(%lu) Unable to connect to server.", GetLastError());
    freeaddrinfo(resolvedAddress);
    return false;
  }

  uint32_t mode = 1;
  ioctlsocket(client->socket, FIONBIO, (u_long*) &mode);

  freeaddrinfo(resolvedAddress);

  return true;
}

Message* tcp_game_client_get_message(TcpGameClient* client)
{
  return game_networking_recv_message_tcp(client->socket);
}

void tcp_game_client_send_message(TcpGameClient* client, const Message* message)
{
  game_networking_send_message_tcp(client->socket, message);
}

void tcp_game_client_destroy(TcpGameClient* client)
{
  shutdown(client->socket, SD_BOTH);
  closesocket(client->socket);
}
