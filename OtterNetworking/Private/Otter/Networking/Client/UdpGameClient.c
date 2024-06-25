#include "Otter/Networking/Client/UdpGameClient.h"

#include "Otter/Networking/Shared/GameNetworking.h"

bool udp_game_client_connect(
    UdpGameClient* client, const char* host, const char* port)
{
  struct addrinfo serverAddress = {.ai_family = AF_INET,
      .ai_socktype                            = SOCK_DGRAM,
      .ai_protocol                            = IPPROTO_UDP,
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

  uint32_t mode = 1;
  ioctlsocket(client->socket, FIONBIO, (u_long*) &mode);

  client->address = *(struct sockaddr_in*) resolvedAddress->ai_addr;

  freeaddrinfo(resolvedAddress);

  return true;
}

Message* udp_game_client_get_message(UdpGameClient* client)
{
  struct sockaddr_in address;
  Message* message = game_networking_recv_message_udp(client->socket, &address);
  if (message != NULL
      && memcmp(&address, &client->address, sizeof(struct sockaddr_in)) != 0)
  {
    fprintf(stderr, "Packet recv'd not from the server.");
    message_destroy(message);
    message = NULL;
  }
  return message;
}

void udp_game_client_send_message(UdpGameClient* client, const Message* message)
{
  game_networking_send_message_udp(client->socket, &client->address, message);
}

void udp_game_client_destroy(UdpGameClient* client)
{
  closesocket(client->socket);
}
