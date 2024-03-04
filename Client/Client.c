#include "Input/Input.h"
#include "Otter/Config/Config.h"
#include "Otter/GameState/GameState.h"
#include "Otter/GameState/Player/Player.h"
#include "Otter/Networking/Client/UdpGameClient.h"
#include "Otter/Networking/Messages/ControlMessages.h"
#include "Otter/Networking/Messages/EntityMessages.h"
#include "Otter/Render/GpuBuffer.h"
#include "Otter/Render/Mesh.h"
#include "Otter/Render/Pipeline.h"
#include "Otter/Render/RenderInstance.h"
#include "Otter/Util/File.h"
#include "Otter/Util/Math/Vec.h"
#include "Window/GameWindow.h"

#define CONFIG_WIDTH  "width"
#define CONFIG_HEIGHT "height"
#define CONFIG_HOST   "host"
#define CONFIG_PORT   "port"

enum ConnectionState
{
  CS_NOT_JOINED,
  CS_JOINING,
  CS_JOINED
};

typedef struct Connection
{
  enum ConnectionState state;
  PlayerInput lastInput;
  LARGE_INTEGER lastHeartbeatTime;
  LARGE_INTEGER lastStateTime;
} Connection;

GUID g_clientGuid;

static uint32_t g_serverTickId;

static LARGE_INTEGER g_timerFrequency;

static void handle_message(Message* message)
{
  switch (message->header->type)
  {
  case MT_PLAYER_ATTRIBUTES:
    {
      PlayerAttributesMessage* payload = message->payload;

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

      g_listOfPlayers[playerPosition].active    = true;
      g_listOfPlayers[playerPosition].id        = payload->playerId;
      g_listOfPlayers[playerPosition].positionX = payload->positionX;
      g_listOfPlayers[playerPosition].positionY = payload->positionY;
      g_listOfPlayers[playerPosition].velocityX = payload->velocityX;
      g_listOfPlayers[playerPosition].velocityY = payload->velocityY;
    }
    break;
  case MT_PLAYER_LEFT:
    {
      PlayerLeftMessage* payload = message->payload;
      for (int i = 0; i < MAX_PLAYERS; i++)
      {
        if (g_listOfPlayers[i].active
            && memcmp(&g_listOfPlayers[i].id, &payload->playerId, sizeof(GUID))
                   == 0)
        {
          g_listOfPlayers[i].active = false;
        }
      }
    }
    break;
  default:
    printf("Warning: Unknown message type %d\n", message->header->type);
    break;
  }

  int64_t ticksMissed =
      (int64_t) message->header->tickId - (int64_t) g_serverTickId;
  if (ticksMissed > 1)
  {
    printf("Missed %lld server ticks!!!\n", ticksMissed - 1);
  }
  else if (ticksMissed < 0)
  {
    printf("Went backwards in time %lld server ticks!!!\n", ticksMissed);
  }
  g_serverTickId = message->header->tickId;
}

static bool processMessages(UdpGameClient* client)
{
  bool messageReceived = false;
  Message* message;
  while ((message = udp_game_client_get_message(client)) != NULL)
  {
    messageReceived = true;
    handle_message(message);
    message_destroy(message);
  }
  return messageReceived;
}

static bool send_heartbeat(UdpGameClient* client)
{
  Message* heartbeat = message_create_heartbeat(&g_clientGuid, g_serverTickId);
  if (heartbeat != NULL)
  {
    udp_game_client_send_message(client, heartbeat);
  }
  else
  {
    MessageBox(NULL, L"Out of memory", L"Internal error", MB_ICONERROR);
    return false;
  }
  return true;
}

static void handle_connection(Connection* connection, UdpGameClient* client)
{
  switch (connection->state)
  {
  case CS_NOT_JOINED:
    {
      QueryPerformanceCounter(&connection->lastStateTime);

      for (int i = 0; i < MAX_PLAYERS; i++)
      {
        g_listOfPlayers[i].active = false;
      }

      printf("Connecting to game...\n");

      Message* joinMessage = message_create_join_request();
      udp_game_client_send_message(client, joinMessage);
      message_destroy(joinMessage);

      connection->state = CS_JOINING;

      break;
    }
  case CS_JOINING:
    {
      LARGE_INTEGER currentTime;
      QueryPerformanceCounter(&currentTime);

      if (((float) (currentTime.QuadPart - connection->lastStateTime.QuadPart)
              / g_timerFrequency.QuadPart)
          < 3.0f)
      {
        Message* reply = NULL;
        while ((reply = udp_game_client_get_message(client)) != NULL)
        {
          if (reply->header->type == MT_JOIN_RESPONSE
              && ((JoinResponseMessage*) reply->payload)->status
                     == JOIN_STATUS_SUCCESS)
          {
            g_clientGuid   = ((JoinResponseMessage*) reply->payload)->clientId;
            g_serverTickId = reply->header->tickId;
            printf("Game joined!\n");

            connection->state = CS_JOINED;

            QueryPerformanceCounter(&connection->lastStateTime);
          }

          message_destroy(reply);
        }
      }
      else
      {
        connection->state = CS_NOT_JOINED;
      }

      break;
    }
  case CS_JOINED:
    {
      LARGE_INTEGER frameStartTime;
      QueryPerformanceCounter(&frameStartTime);

      float stateDeltaTime =
          (float) (frameStartTime.QuadPart - connection->lastStateTime.QuadPart)
          / g_timerFrequency.QuadPart;
      if (processMessages(client))
      {
        connection->lastStateTime = frameStartTime;
      }
      else if (stateDeltaTime > 5.0f)
      {
        connection->state = CS_NOT_JOINED;
      }

      float deltaTime = (float) (frameStartTime.QuadPart
                                 - connection->lastHeartbeatTime.QuadPart)
                      / g_timerFrequency.QuadPart;
      if (deltaTime > 0.5f)
      {
        if (!send_heartbeat(client))
        {
          break;
        }
        connection->lastHeartbeatTime = frameStartTime;
      }

      if (connection->lastInput.actions != g_input.actions)
      {
        Message* moveMessage = message_create_player_move(
            &g_clientGuid, &g_clientGuid, g_input, g_serverTickId);
        udp_game_client_send_message(client, moveMessage);

        connection->lastInput = g_input;
      }
      break;
    }
  }
}

int main()
{
  wWinMain(GetModuleHandle(NULL), NULL, L"", 1);
}

int WINAPI wWinMain(
    HINSTANCE instance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow)
{
  (void) instance;
  (void) prevInstance;
  (void) cmdLine;
  (void) cmdShow;

  QueryPerformanceFrequency(&g_timerFrequency);

  char* configStr = file_load("Config/client.ini", NULL);
  if (configStr == NULL)
  {
    fprintf(stderr, "Unable to find file config.ini\n");
    return -1;
  }

  HashMap* config = config_parse(configStr);
  free(configStr);
  if (config == NULL)
  {
    fprintf(stderr, "Could not parse configuration.");
    return -1;
  }

  int width  = atoi((const char*) hash_map_get_value(config, CONFIG_WIDTH));
  int height = atoi((const char*) hash_map_get_value(config, CONFIG_HEIGHT));

  HWND window = game_window_create(width, height, WM_WINDOWED);
  RenderInstance* renderInstance = render_instance_create(window);
  if (renderInstance == NULL)
  {
    fprintf(stderr, "Failed to initialize render instance.\n");
    game_window_destroy(window);
    hash_map_destroy(config);
    return -1;
  }

  // TODO: make this better.
  // ------
  const MeshVertex vertices[] = {{.position = {-0.5f, -0.5f, 0.5f}},
      {.position = {0.5f, -0.5f, 0.5f}}, {.position = {-0.5f, 0.5f, 0.5f}},
      {.position = {0.5f, 0.5f, 0.5f}}, {.position = {-0.5f, -0.5f, -0.5f}},
      {.position = {0.5f, -0.5f, -0.5f}}, {.position = {-0.5f, 0.5f, -0.5f}},
      {.position = {0.5f, 0.5f, -0.5f}}};
  const uint16_t indices[]    = {
      0, 2, 1, 2, 3, 1, // Front
      1, 3, 5, 3, 7, 5, // Right
      5, 7, 4, 7, 6, 4, // Back
      4, 6, 0, 6, 2, 0, // Left
      4, 0, 1, 4, 1, 5, // Top
      2, 6, 3, 6, 7, 3 // Bottom
  };

  VkQueue graphicsQueue;
  vkGetDeviceQueue(renderInstance->logicalDevice,
      renderInstance->graphicsQueueFamily, 0, &graphicsQueue);

  Mesh* cube =
      mesh_create(vertices, _countof(vertices), indices, _countof(indices),
          renderInstance->physicalDevice, renderInstance->logicalDevice,
          renderInstance->commandPool, graphicsQueue);

  Pipeline* pipeline = pipeline_create(
      "pbr", renderInstance->logicalDevice, renderInstance->renderPass);

  renderInstance->command.vertices     = cube->vertices->buffer;
  renderInstance->command.indices      = cube->indices->buffer;
  renderInstance->command.numOfIndices = cube->indices->size / sizeof(uint16_t);
  renderInstance->command.pipeline     = pipeline->pipeline;
  // ------

  char* host = hash_map_get_value(config, CONFIG_HOST);
  char* port = hash_map_get_value(config, CONFIG_PORT);
  UdpGameClient client;
  if (!udp_game_client_connect(&client, host, port))
  {
    MessageBox(window, L"Unable to connect to server.", L"Connection Issue!",
        MB_ICONERROR);
    render_instance_destroy(renderInstance);
    game_window_destroy(window);
    hash_map_destroy(config);
    return -1;
  }

  Connection connection = {.state = CS_NOT_JOINED,
      .lastInput                  = {0},
      .lastHeartbeatTime          = 0,
      .lastStateTime              = 0};

  QueryPerformanceCounter(&connection.lastHeartbeatTime);
  connection.lastStateTime    = connection.lastHeartbeatTime;
  LARGE_INTEGER lastFrameTime = connection.lastStateTime;
  while (!game_window_process_message())
  {
    handle_connection(&connection, &client);

    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    float deltaTime = ((float) (currentTime.QuadPart - lastFrameTime.QuadPart)
                       / g_timerFrequency.QuadPart);
    game_state_update(NULL, deltaTime);

    render_instance_draw(renderInstance);

    lastFrameTime = currentTime;
  }

  if (connection.state == CS_JOINED)
  {
    Message* disconnectMessage = message_create_leave_request(&g_clientGuid);
    if (disconnectMessage != NULL)
    {
      udp_game_client_send_message(&client, disconnectMessage);
      message_destroy(disconnectMessage);
    }
  }

  pipeline_destroy(pipeline, renderInstance->logicalDevice);
  mesh_destroy(cube, renderInstance->logicalDevice);
  udp_game_client_destroy(&client);
  render_instance_destroy(renderInstance);
  game_window_destroy(window);
  hash_map_destroy(config);

  return 0;
}
