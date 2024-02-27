#pragma once

#include "Otter/GameState/export.h"

#define PLAYER_SPEED 50.0f
#define MAX_PLAYERS 16

// TODO: Make struct of lists for better cache utilization.
typedef struct Player
{
  GUID id;
  float x;
  float y;
  bool active;
} Player;

OTTER_API extern Player g_listOfPlayers[MAX_PLAYERS];
