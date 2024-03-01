#include "Server.h"

#include "Otter/Config/Config.h"
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

static void broadcast_game_state(UdpGameServer* server)
{
  Message* playerUpdate =
      message_create_player_attributes(&g_serverGuid, g_serverTickId);
  PlayerAttributesMessage* payload = playerUpdate->payload;

  for (int i = 0; i < MAX_PLAYERS; i++)
  {
    if (g_listOfPlayers[i].active)
    {
      payload->playerId  = g_listOfPlayers[i].id;
      payload->positionX = g_listOfPlayers[i].positionX;
      payload->positionY = g_listOfPlayers[i].positionY;
      payload->velocityX = g_listOfPlayers[i].velocityX;
      payload->velocityY = g_listOfPlayers[i].velocityY;

      udp_game_server_broadcast_message(server, playerUpdate);
    }
  }
}

static void disconnect_player(UdpGameServer* server, Player* player)
{
  player->active = false;
  udp_game_server_disconnect_client(server, &player->id);
  wchar_t guid[128];
  if (StringFromGUID2(&player->id, guid, sizeof(guid) / sizeof(guid[0])) == 0)
  {
    wcscpy_s(guid, sizeof(guid) / sizeof(guid[0]), L"(unknown)");
  }
  wprintf(L"Player %s left the game\n", guid);
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
      disconnect_player(server, &g_listOfPlayers[i]);
    }
  }
}

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
          g_listOfPlayers[i].active    = true;
          g_listOfPlayers[i].positionX = 50.0f;
          g_listOfPlayers[i].positionY = 50.0f;
          g_listOfPlayers[i].id        = requestMessage->header->entity;

          wchar_t guid[128];
          if (StringFromGUID2(
                  &g_listOfPlayers[i].id, guid, sizeof(guid) / sizeof(guid[0]))
              == 0)
          {
            wcscpy_s(guid, sizeof(guid) / sizeof(guid[0]), L"(unknown)");
          }
          wprintf(L"Player %s joined the game\n", guid);

          Message* responseMessage = message_create_join_response(&g_serverGuid,
              JOIN_STATUS_SUCCESS, &g_listOfPlayers[i].id, g_serverTickId);
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
        GUID id                  = {0};
        Message* responseMessage = message_create_join_response(&g_serverGuid,
            JOIN_STATUS_FAILED_TOO_MANY_PLAYERS, &id, g_serverTickId);
        udp_game_server_send_message(
            server, &requestMessage->header->entity, responseMessage);
        message_destroy(responseMessage);
      }
    }
    break;
  case MT_LEAVE_REQUEST:
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
      if (g_listOfPlayers[i].active
          && memcmp(&g_listOfPlayers[i].id, &requestMessage->header->entity,
                 sizeof(GUID))
                 == 0)
      {
        disconnect_player(server, &g_listOfPlayers[i]);

        Message* disconnectMessage = message_create_player_left(
            &g_serverGuid, &requestMessage->header->entity, g_serverTickId);
        if (disconnectMessage != NULL)
        {
          udp_game_server_broadcast_message(server, disconnectMessage);
          message_destroy(disconnectMessage);
        }
        else
        {
          MessageBox(NULL, L"Out of memory", L"Internal error", MB_ICONERROR);
        }
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

static char* load_file(const char* path)
{
  HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
      OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file == INVALID_HANDLE_VALUE)
  {
    return NULL;
  }

  int fileSize = GetFileSize(file, NULL);

  char* text = malloc(fileSize + 1);
  if (text == NULL)
  {
    CloseHandle(file);
    return NULL;
  }

  if (!ReadFile(file, text, fileSize, NULL, NULL))
  {
    free(text);
    text = NULL;
  }
  text[fileSize] = '\0';

  CloseHandle(file);
  return text;
}

int main(int argc, char** argv)
{
  char* configStr = load_file("Config/config.ini");
  if (configStr == NULL)
  {
    return -1;
  }

  HashMap* config = config_parse(configStr);
  free(configStr);
  if (config == NULL)
  {
    return -1;
  }

  char* ip   = hash_map_get_value(config, "ip");
  char* port = hash_map_get_value(config, "port");

  UdpGameServer server;
  if (!udp_game_server_create(&server, ip, port))
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
  hash_map_destroy(config);

  return 0;
}
