#pragma once

#include "Otter/Networking/Messages/Message.h"
#include "Otter/Networking/export.h"

typedef struct TcpGameClient
{
  SOCKET socket;
} TcpGameClient;

OTTER_API bool tcp_game_client_connect(
    TcpGameClient* client, const char* host, const char* port);

OTTER_API Message* tcp_game_client_get_message(TcpGameClient* client);

OTTER_API void tcp_game_client_send_message(
    TcpGameClient* client, const Message* message);

OTTER_API void tcp_game_client_destroy(TcpGameClient* client);
