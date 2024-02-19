#pragma once

#include "Otter/Networking/Messages/Message.h"
#include "Otter/Networking/export.h"

#define MAX_CLIENTS   16
#define ALL_CLIENT_ID -1

typedef void (*GameServerDisconnectCb)(int clientId);

typedef struct GameServer
{
  SOCKET socket;
  SOCKET clients[MAX_CLIENTS];
  bool running;
  HANDLE acceptThread;
  GameServerDisconnectCb disconnectCb;
} GameServer;

OTTER_API bool game_server_create(GameServer* server, const char* host,
    const char* port, GameServerDisconnectCb disconnectCb);

OTTER_API Message* game_server_get_message(GameServer* server, int* clientId);

OTTER_API void game_server_send_message(
    GameServer* server, int clientId, Message* message);

OTTER_API void game_server_destroy(GameServer* server);

OTTER_API void game_server_disconnect_client(GameServer* server, int clientId);
