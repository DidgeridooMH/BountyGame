#pragma once

#include "Otter/Networking/Messages/Message.h"
#include "Otter/Networking/export.h"

OTTER_API Message* game_networking_recv_message(SOCKET socket);
OTTER_API void game_networking_send_message(SOCKET socket, Message* message);
