#pragma once

#include "Otter/Networking/Messages/Message.h"
#include "Otter/Networking/export.h"

typedef struct UdpGameClient
{
  SOCKET socket;
  struct sockaddr_in address;
} UdpGameClient;

OTTER_API bool udp_game_client_connect(
    UdpGameClient* client, const char* host, const char* port);

OTTER_API Message* udp_game_client_get_message(UdpGameClient* client);

OTTER_API void udp_game_client_send_message(
    UdpGameClient* client, const Message* message);

OTTER_API void udp_game_client_destroy(UdpGameClient* client);
