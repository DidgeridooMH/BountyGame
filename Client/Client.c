#include "Input/Input.h"
#include "Otter/GameState/Player/Player.h"
#include "Otter/Networking/Client/GameClient.h"
#include "Otter/Networking/Messages/ControlMessages.h"
#include "Otter/Networking/Messages/EntityMessages.h"
#include "Window/GameWindow.h"

static void handle_message(Message* message)
{
  switch (message->header.type)
  {
  case MT_PLAYER_POSITION:
    {
      PlayerPositionMessage* payload = message->payload;

      int playerPosition = -1;
      for (int i = 0; i < MAX_PLAYERS; i++)
      {
        if (g_listOfPlayers[i].active)
        {
          if (memcmp(&g_listOfPlayers[i].id, &payload->playerId, sizeof(GUID))
              == 0)
          {
            playerPosition = i;
            break;
          }
        }
        else if (playerPosition == -1)
        {
          playerPosition = i;
        }
      }

      g_listOfPlayers[playerPosition].active = true;
      g_listOfPlayers[playerPosition].id     = payload->playerId;
      g_listOfPlayers[playerPosition].x      = payload->x;
      g_listOfPlayers[playerPosition].y      = payload->y;
    }
    break;
  default:
    break;
  }
}

GUID g_clientGuid;

int WINAPI wWinMain(
    HINSTANCE instance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow)
{
  (void) instance;
  (void) prevInstance;
  (void) cmdLine;
  (void) cmdShow;

  LARGE_INTEGER timerFrequency;
  QueryPerformanceFrequency(&timerFrequency);

  HWND window = game_window_create();

  GameClient client;
  if (!game_client_connect(&client, "192.168.1.29", "42003"))
  {
    MessageBox(window, L"Unable to connect to server at localhost:42003.",
        L"Connection Issue!", MB_ICONERROR);
    game_window_destroy(window);
    return -1;
  }

  CoCreateGuid(&g_clientGuid);

  Message message = {.header = {.entity = g_clientGuid,
                         .payloadSize   = sizeof(JoinRequestMessage),
                         .type          = MT_JOIN_REQUEST},
      .payload               = malloc(sizeof(JoinRequestMessage))};
  if (message.payload == NULL)
  {
    printf("Out of memory\n");
    game_client_destroy(&client);
    game_window_destroy(window);
    return -1;
  }
  game_client_send_message(&client, &message);
  free(message.payload);

  // TODO: This should async and have a couple seconds of wait time.
  Message* reply = NULL;
  int timeout    = 0;
  while ((reply = game_client_get_message(&client)) == NULL && timeout < 3)
  {
    Sleep(1000);
    timeout += 1;
  }

  if (reply == NULL)
  {
    printf("No join response found. Oops\n");
    game_client_destroy(&client);
    game_window_destroy(window);
    return -1;
  }

  if (reply->header.type == MT_JOIN_RESPONSE
      && ((JoinResponseMessage*) reply->payload)->code == JOIN_STATUS_SUCCESS)
  {
    printf("Game joined!");
  }
  else
  {
    MessageBox(window, L"Server full.", L"Unable to join server", MB_ICONERROR);
    return -1;
  }

  PlayerInput lastInput = {0};
  LARGE_INTEGER startTime;
  QueryPerformanceCounter(&startTime);
  while (!game_window_process_message())
  {
    Message* message;
    // TODO: This needs to split to a separate thread.
    if ((message = game_client_get_message(&client)) != NULL)
    {
      handle_message(message);
      free(message->payload);
      free(message);
    }

    LARGE_INTEGER frameStartTime;
    QueryPerformanceCounter(&frameStartTime);

    float deltaTime = (float) (frameStartTime.QuadPart - startTime.QuadPart)
                    / timerFrequency.QuadPart;

    if (lastInput.actions != g_input.actions)
    {
      Message moveMessage            = {0};
      moveMessage.header.entity      = g_clientGuid;
      moveMessage.header.payloadSize = sizeof(PlayerMoveMessage);
      moveMessage.header.type        = MT_PLAYER_MOVE;
      PlayerMoveMessage payload      = {
               .playerId = g_clientGuid, .direction = g_input};
      moveMessage.payload = &payload;
      game_client_send_message(&client, &moveMessage);

      lastInput = g_input;
    }

    startTime = frameStartTime;
  }

  game_client_destroy(&client);
  game_window_destroy(window);

  return 0;
}
