#pragma once

#include "Otter/Networking/Messages/Message.h"
#include "Otter/Networking/export.h"

#define MAX_CLIENTS   16
#define ALL_CLIENT_ID -1

typedef void (*GameServerDisconnectCb)(int clientId);

typedef struct TcpGameServer
{
  SOCKET socket;
  SOCKET clients[MAX_CLIENTS];
  bool running;
  HANDLE acceptThread;
  GameServerDisconnectCb disconnectCb;
} TcpGameServer;

OTTER_API bool tcp_game_server_create(TcpGameServer* server, const char* host,
    const char* port, GameServerDisconnectCb disconnectCb);

OTTER_API Message* tcp_game_server_get_message(
    TcpGameServer* server, int* clientId);

OTTER_API void tcp_game_server_send_message(
    TcpGameServer* server, int clientId, const Message* message);

OTTER_API void tcp_game_server_destroy(TcpGameServer* server);

OTTER_API void tcp_game_server_disconnect_client(
    TcpGameServer* server, int clientId);
