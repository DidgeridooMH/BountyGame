#include "GameState.h"

#include "Player/Player.h"

void game_state_update(PlayerInput* playerInputs, float deltaTime)
{
  for (int i = 0; i < MAX_PLAYERS; i++)
  {
    if (g_listOfPlayers[i].active && playerInputs[i].actions > 0)
    {
      float deltaX = playerInputs[i].left * -1 + playerInputs[i].right;
      float deltaY = playerInputs[i].up * -1 + playerInputs[i].down;

      g_listOfPlayers[i].x += deltaX * deltaTime * PLAYER_SPEED;
      g_listOfPlayers[i].y += deltaY * deltaTime * PLAYER_SPEED;
    }
  }
}