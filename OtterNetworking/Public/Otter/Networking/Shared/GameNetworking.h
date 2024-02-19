#pragma once

#include "Otter/Networking/Messages/Message.h"
#include "Otter/Networking/export.h"

OTTER_API void game_networking_send_message_tcp(
    SOCKET socket, const Message* message);

OTTER_API void game_networking_send_message_udp(
    SOCKET socket, const struct sockaddr_in* target, const Message* message);

OTTER_API Message* game_networking_recv_message_tcp(SOCKET socket);

OTTER_API Message* game_networking_recv_message_udp(
    SOCKET socket, struct sockaddr_in* address);
