#pragma once

#include "Otter/Networking/Messages/Message.h"
#include "Otter/Networking/export.h"

#define MAX_CLIENTS 16

typedef void (*GameServerDisconnectCb)(int clientId);

typedef struct UdpPeer
{
  bool connected;
  GUID id;
  struct sockaddr_in address;
} UdpPeer;

typedef struct UdpGameServer
{
  SOCKET socket;
  UdpPeer clients[16];
} UdpGameServer;

OTTER_API bool udp_game_server_create(
    UdpGameServer* server, const char* host, const char* port);

OTTER_API Message* udp_game_server_get_message(UdpGameServer* server);

OTTER_API void udp_game_server_send_message(
    UdpGameServer* server, GUID* clientId, Message* message);

OTTER_API void udp_game_server_broadcast_message(
    UdpGameServer* server, Message* message);

OTTER_API void udp_game_server_destroy(UdpGameServer* server);

OTTER_API void udp_game_server_disconnect_client(
    UdpGameServer* server, GUID* clientId);
