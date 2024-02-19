#include "Otter/GameState/GameState.h"
#include "Otter/GameState/Player/Player.h"
#include "Otter/GameState/Player/PlayerInput.h"
#include "Otter/Networking/Messages/ControlMessages.h"
#include "Otter/Networking/Messages/EntityMessages.h"
#include "Otter/Networking/Server/GameServer.h"

typedef struct Client
{
  GUID id;
  int networkId;
  bool connected;
} Client;

static bool g_running = true;

static Client g_clientList[MAX_PLAYERS];
static PlayerInput g_playerInput[MAX_PLAYERS];

static void handle_client_disconnected(int clientId)
{
  for (int i = 0; i < 16; i++)
  {
    if (g_clientList[i].connected && g_clientList[i].networkId == clientId)
    {
      wchar_t guid[128];
      if (StringFromGUID2(
              &g_clientList[i].id, guid, sizeof(guid) / sizeof(guid[0]))
          == 0)
      {
        wcscpy_s(guid, sizeof(guid) / sizeof(guid[0]), L"(unknown)");
      }
      wprintf(L"Player %s left the game\n", guid);

      g_clientList[i].connected = false;
      g_listOfPlayers[i].active = false;
    }
  }
}

static void handle_message(
    GameServer* server, Message* requestMessage, int clientId)
{
  switch (requestMessage->header.type)
  {
  case MT_JOIN_REQUEST:
    {
      Message responseMessage = {0};
      memset(&responseMessage.header.entity, 0, sizeof(GUID));
      responseMessage.header.type        = MT_JOIN_RESPONSE;
      responseMessage.header.payloadSize = sizeof(JoinResponseMessage);

      JoinResponseMessage payload = {0};
      responseMessage.payload     = &payload;

      int playerJoined = false;
      for (int i = 0; i < MAX_PLAYERS; i++)
      {
        if (!g_clientList[i].connected)
        {
          g_clientList[i].connected = true;
          g_clientList[i].networkId = clientId;
          g_clientList[i].id        = requestMessage->header.entity;

          g_listOfPlayers[i].active = true;
          g_listOfPlayers[i].id     = requestMessage->header.entity;
          g_listOfPlayers[i].x      = 50.0f;
          g_listOfPlayers[i].y      = 50.0f;

          wchar_t guid[128];
          if (StringFromGUID2(
                  &g_clientList[i].id, guid, sizeof(guid) / sizeof(guid[0]))
              == 0)
          {
            wcscpy_s(guid, sizeof(guid) / sizeof(guid[0]), L"(unknown)");
          }
          wprintf(L"Player %s joined the game\n", guid);

          payload.code = JOIN_STATUS_SUCCESS;
          game_server_send_message(server, clientId, &responseMessage);
          playerJoined = true;
          break;
        }
      }

      if (!playerJoined)
      {
        payload.code = JOIN_STATUS_FAILED_TOO_MANY_PLAYERS;
        game_server_send_message(server, clientId, &responseMessage);
        game_server_disconnect_client(server, clientId);
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
  default:
    break;
  }
}

int main(int argc, char** argv)
{
  GameServer server;
  if (!game_server_create(
          &server, "0.0.0.0", "42003", handle_client_disconnected))
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
    int clientId            = 0;
    if ((requestMessage = game_server_get_message(&server, &clientId)) != NULL)
    {
      handle_message(&server, requestMessage, clientId);
      free(requestMessage->payload);
      free(requestMessage);
    }

    LARGE_INTEGER frameStartTime;
    QueryPerformanceCounter(&frameStartTime);

    float deltaTime = (float) (frameStartTime.QuadPart - startTime.QuadPart)
                    / timerFrequency.QuadPart;
    game_state_update(g_playerInput, deltaTime);
    startTime = frameStartTime;

    LARGE_INTEGER clientSendEndTime;
    QueryPerformanceCounter(&clientSendEndTime);
    if (((clientSendEndTime.QuadPart - clientSendTime.QuadPart) * 1000.0f
            / timerFrequency.QuadPart)
        > (1000.0f / 64.0f))
    {
      Message playerUpdate = {0};
      memset(&playerUpdate.header.entity, 0, sizeof(GUID));
      playerUpdate.header.type        = MT_PLAYER_POSITION;
      playerUpdate.header.payloadSize = sizeof(PlayerPositionMessage);

      PlayerPositionMessage payload = {0};
      playerUpdate.payload          = &payload;

      for (int i = 0; i < MAX_PLAYERS; i++)
      {
        if (g_listOfPlayers[i].active)
        {
          payload.playerId = g_listOfPlayers[i].id;
          payload.x        = g_listOfPlayers[i].x;
          payload.y        = g_listOfPlayers[i].y;

          game_server_send_message(&server, ALL_CLIENT_ID, &playerUpdate);
        }
      }
      QueryPerformanceCounter(&clientSendTime);
    }
  }

  game_server_destroy(&server);

  return 0;
}
