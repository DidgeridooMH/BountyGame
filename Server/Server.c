#include "Server.h"

#include "Otter/GameState/GameState.h"
#include "Otter/GameState/Player/Player.h"
#include "Otter/GameState/Player/PlayerInput.h"
#include "Otter/Networking/Messages/ControlMessages.h"
#include "Otter/Networking/Messages/EntityMessages.h"
#include "Otter/Networking/Server/UdpGameServer.h"

GUID g_serverGuid = {0, 0, 0, 0};

static uint32_t g_serverTickId = 0;

static bool g_running = true;

static LARGE_INTEGER g_playerHeartbeat[MAX_PLAYERS];
static PlayerInput g_playerInput[MAX_PLAYERS];

static void handle_message(UdpGameServer* server, Message* requestMessage)
{
  switch (requestMessage->header->type)
  {
  case MT_JOIN_REQUEST:
    {
      int playerJoined = false;
      for (int i = 0; i < MAX_PLAYERS; i++)
      {
        if (!g_listOfPlayers[i].active)
        {
          g_listOfPlayers[i].active = true;
          g_listOfPlayers[i].id     = requestMessage->header->entity;
          g_listOfPlayers[i].x      = 50.0f;
          g_listOfPlayers[i].y      = 50.0f;

          wchar_t guid[128];
          if (StringFromGUID2(
                  &g_listOfPlayers[i].id, guid, sizeof(guid) / sizeof(guid[0]))
              == 0)
          {
            wcscpy_s(guid, sizeof(guid) / sizeof(guid[0]), L"(unknown)");
          }
          wprintf(L"Player %s joined the game\n", guid);

          Message* responseMessage = message_create_join_response(
              &g_serverGuid, JOIN_STATUS_SUCCESS, g_serverTickId);
          udp_game_server_send_message(
              server, &g_listOfPlayers[i].id, responseMessage);
          message_destroy(responseMessage);

          QueryPerformanceCounter(&g_playerHeartbeat[i]);
          playerJoined = true;
          break;
        }
      }

      if (!playerJoined)
      {
        Message* responseMessage = message_create_join_response(
            &g_serverGuid, JOIN_STATUS_FAILED_TOO_MANY_PLAYERS, g_serverTickId);
        udp_game_server_send_message(
            server, &requestMessage->header->entity, responseMessage);
        message_destroy(responseMessage);
      }
    }
    break;
  case MT_PLAYER_MOVE:
    {
      PlayerMoveMessage* moveMessage = requestMessage->payload;
      for (int i = 0; i < MAX_PLAYERS; i++)
      {
        if (g_listOfPlayers[i].active
            && memcmp(
                   &g_listOfPlayers[i].id, &moveMessage->playerId, sizeof(GUID))
                   == 0)
        {
          g_playerInput[i] = moveMessage->direction;
          break;
        }
      }
    }
    break;
  case MT_HEARTBEAT:
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
      if (g_listOfPlayers[i].active
          && memcmp(&g_listOfPlayers[i].id, &requestMessage->header->entity,
                 sizeof(GUID))
                 == 0)
      {
        QueryPerformanceCounter(&g_playerHeartbeat[i]);
        break;
      }
    }
    break;
  default:
    break;
  }
}

static void broadcast_game_state(UdpGameServer* server)
{
  Message* playerUpdate =
      message_create_player_position(&g_serverGuid, g_serverTickId);
  PlayerPositionMessage* payload = playerUpdate->payload;

  for (int i = 0; i < MAX_PLAYERS; i++)
  {
    if (g_listOfPlayers[i].active)
    {
      payload->playerId = g_listOfPlayers[i].id;
      payload->x        = g_listOfPlayers[i].x;
      payload->y        = g_listOfPlayers[i].y;

      udp_game_server_broadcast_message(server, playerUpdate);
    }
  }
}

static void handle_player_disconnect(
    UdpGameServer* server, LONGLONG currentTime, LONGLONG timerFrequency)
{
  for (int i = 0; i < MAX_PLAYERS; i++)
  {
    if (g_listOfPlayers[i].active
        && (float) (currentTime - g_playerHeartbeat->QuadPart) / timerFrequency
               > 3.0f)
    {
      g_listOfPlayers[i].active = false;
      udp_game_server_disconnect_client(server, &g_listOfPlayers[i].id);
      wchar_t guid[128];
      if (StringFromGUID2(
              &g_listOfPlayers[i].id, guid, sizeof(guid) / sizeof(guid[0]))
          == 0)
      {
        wcscpy_s(guid, sizeof(guid) / sizeof(guid[0]), L"(unknown)");
      }
      wprintf(L"Player %s left the game\n", guid);
    }
  }
}

int main(int argc, char** argv)
{
  UdpGameServer server;
  if (!udp_game_server_create(&server, "0.0.0.0", "42003"))
  {
    return 1;
  }
  printf("Game server created\n");

  LARGE_INTEGER timerFrequency;
  QueryPerformanceFrequency(&timerFrequency);

  LARGE_INTEGER clientSendTime;
  QueryPerformanceCounter(&clientSendTime);

  LARGE_INTEGER startTime;
  QueryPerformanceCounter(&startTime);
  while (g_running)
  {
    Message* requestMessage = NULL;
    while ((requestMessage = udp_game_server_get_message(&server)) != NULL)
    {
      handle_message(&server, requestMessage);
      message_destroy(requestMessage);
    }

    LARGE_INTEGER frameStartTime;
    QueryPerformanceCounter(&frameStartTime);

    float deltaTime = (float) (frameStartTime.QuadPart - startTime.QuadPart)
                    / timerFrequency.QuadPart;
    game_state_update(g_playerInput, deltaTime);
    startTime = frameStartTime;

    LARGE_INTEGER clientSendEndTime;
    QueryPerformanceCounter(&clientSendEndTime);

    float timeSinceLastBroadcastSec =
        (float) (clientSendEndTime.QuadPart - clientSendTime.QuadPart)
        / timerFrequency.QuadPart;
    if (timeSinceLastBroadcastSec > 1.0f / 128.0f)
    {
      handle_player_disconnect(
          &server, clientSendTime.QuadPart, timerFrequency.QuadPart);

      broadcast_game_state(&server);

      clientSendTime = clientSendEndTime;

      g_serverTickId += 1;
    }
  }

  udp_game_server_destroy(&server);

  return 0;
}
