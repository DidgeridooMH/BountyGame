#include "Otter/GameState/GameState.h"

#include "Otter/GameState/Player/Player.h"

void game_state_update(PlayerInput* playerInputs, float deltaTime)
{
  for (int i = 0; i < MAX_PLAYERS; i++)
  {
    if (g_listOfPlayers[i].active)
    {
      if (playerInputs != NULL)
      {
        float vx = (float) (playerInputs[i].left * -1 + playerInputs[i].right);
        float vy = (float) (playerInputs[i].up * -1 + playerInputs[i].down);

        if (vx != 0 && vy != 0)
        {
          float w = sqrtf(vx * vx + vy * vy);
          vx /= w;
          vy /= w;
        }

        g_listOfPlayers[i].velocityX = PLAYER_SPEED * vx;
        g_listOfPlayers[i].velocityY = PLAYER_SPEED * vy;
      }

      g_listOfPlayers[i].positionX += g_listOfPlayers[i].velocityX * deltaTime;
      g_listOfPlayers[i].positionY += g_listOfPlayers[i].velocityY * deltaTime;
    }
  }
}
