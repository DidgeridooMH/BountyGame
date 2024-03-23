#pragma once

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WIN32_LEAN_AND_MEAN
// WinSock2 headers must be ordered in a specific way.
// clang-format off
#include <winsock2.h>
#include <WS2tcpip.h>
// clang-format on

#include <Objbase.h>
#include <shellscalingapi.h>

#include <Windows.h>
