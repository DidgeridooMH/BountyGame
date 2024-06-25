#include "Otter/Networking/Server/UdpGameServer.h"

#include "Otter/Networking/Shared/GameNetworking.h"

bool udp_game_server_create(
    UdpGameServer* server, const char* host, const char* port)
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

  server->socket = socket(resolvedAddress->ai_family,
      resolvedAddress->ai_socktype, resolvedAddress->ai_protocol);

  if (server->socket == INVALID_SOCKET)
  {
    fprintf(stderr, "(%lu) Unable to open socket.", GetLastError());
    freeaddrinfo(resolvedAddress);
    return false;
  }

  uint32_t mode = 1;
  ioctlsocket(server->socket, FIONBIO, (u_long*) &mode);

  if (bind(server->socket, resolvedAddress->ai_addr,
          (int) resolvedAddress->ai_addrlen))
  {
    fprintf(stderr, "(%lu) Unable to bind socket.", GetLastError());
    freeaddrinfo(resolvedAddress);
    return false;
  }

  freeaddrinfo(resolvedAddress);

  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    server->clients[i].connected = false;
  }

  return true;
}

Message* udp_game_server_get_message(UdpGameServer* server)
{
  struct sockaddr_in address = {0};
  Message* message = game_networking_recv_message_udp(server->socket, &address);
  if (message != NULL)
  {
    int freeSpace = -1;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
      if (server->clients[i].connected)
      {
        if (memcmp(&server->clients[i].address, &address,
                sizeof(struct sockaddr_in))
                == 0
            && memcmp(&server->clients[i].id, &message->header->entity,
                   sizeof(GUID))
                   == 0)
        {
          return message;
        }
      }
      else if (message->header->type == MT_JOIN_REQUEST)
      {
        freeSpace = i;
      }
    }

    // TODO: Look at a matchmaking list to make sure that the client actually
    // has permission to join. We could probably use a token returned from the
    // matchmaker to each client that gives permission and client information.
    if (freeSpace >= 0)
    {
      if (FAILED(CoCreateGuid(&server->clients[freeSpace].id)))
      {
        return NULL;
      }
      message->header->entity              = server->clients[freeSpace].id;
      server->clients[freeSpace].address   = address;
      server->clients[freeSpace].connected = true;
    }
  }
  return message;
}

void udp_game_server_send_message(
    UdpGameServer* server, GUID* clientId, Message* message)
{
  for (int i = 0; i <= MAX_CLIENTS; i++)
  {
    if (i == MAX_CLIENTS)
    {
      printf("WARN: Failed to send to player.\n");
      break;
    }

    if (server->clients[i].connected
        && memcmp(&server->clients[i].id, clientId, sizeof(GUID)) == 0)
    {
      game_networking_send_message_udp(
          server->socket, &server->clients[i].address, message);
      break;
    }
  }
}

void udp_game_server_broadcast_message(UdpGameServer* server, Message* message)
{
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (server->clients[i].connected)
    {
      game_networking_send_message_udp(
          server->socket, &server->clients[i].address, message);
    }
  }
}

void udp_game_server_destroy(UdpGameServer* server)
{
  closesocket(server->socket);
}

void udp_game_server_disconnect_client(UdpGameServer* server, GUID* clientId)
{
  for (int i = 0; i < MAX_CLIENTS; i++)
  {
    if (server->clients[i].connected
        && memcmp(clientId, &server->clients[i].id, sizeof(GUID)) == 0)
    {
      server->clients[i].connected = false;
      break;
    }
  }
}
