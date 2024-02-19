#pragma once

#include "Otter/Networking/Messages/Message.h"
#include "Otter/Networking/export.h"

typedef struct GameClient
{
  SOCKET socket;
} GameClient;

OTTER_API bool game_client_connect(
    GameClient* client, const char* host, const char* port);

OTTER_API Message* game_client_get_message(GameClient* client);

OTTER_API void game_client_send_message(GameClient* client, Message* message);

OTTER_API void game_client_destroy(GameClient* client);
