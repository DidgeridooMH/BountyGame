#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
// WinSock2 headers must be ordered in a specific way.
// clang-format off
#include <winsock2.h>
#include <WS2tcpip.h>
// clang-format on

#include <objbase.h>

#include <Windows.h>
