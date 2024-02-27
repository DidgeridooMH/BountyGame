#pragma once

#include "Otter/GameState/export.h"

#define PLAYER_SPEED 100.0f
#define MAX_PLAYERS 16

// TODO: Make struct of lists for better cache utilization.
typedef struct Player
{
  bool active;
  GUID id;
  float positionX;
  float positionY;
  float velocityX;
  float velocityY;
} Player;

OTTER_API extern Player g_listOfPlayers[MAX_PLAYERS];
