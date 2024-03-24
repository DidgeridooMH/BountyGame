#include "Input/Input.h"
#include "Otter/Config/Config.h"
#include "Otter/GameState/GameState.h"
#include "Otter/GameState/Player/Player.h"
#include "Otter/Math/Transform.h"
#include "Otter/Math/Vec.h"
#include "Otter/Networking/Client/UdpGameClient.h"
#include "Otter/Networking/Messages/ControlMessages.h"
#include "Otter/Networking/Messages/EntityMessages.h"
#include "Otter/Render/Gltf/GlbAsset.h"
#include "Otter/Render/Mesh.h"
#include "Otter/Render/RenderInstance.h"
#include "Otter/Util/File.h"
#include "Otter/Util/Profiler.h"
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

  profiler_init(g_timerFrequency);

  size_t fileLength = 0;
  char* glbTest     = file_load("model.glb", &fileLength);
  if (glbTest == NULL)
  {
    fprintf(stderr, "Unable to find file model.glb\n");
    profiler_destroy();
    return -1;
  }

  GlbAsset asset;
  if (!glb_load_asset(glbTest, fileLength, &asset))
  {
    fprintf(stderr, "Unable to parse model.glb\n");
    profiler_destroy();
    return -1;
  }

  char* configStr = file_load("Config/client.ini", NULL);
  if (configStr == NULL)
  {
    fprintf(stderr, "Unable to find file config.ini\n");
    profiler_destroy();
    return -1;
  }

  HashMap config;
  if (!config_parse(&config, configStr))
  {
    free(configStr);
    fprintf(stderr, "Could not parse configuration.");
    profiler_destroy();
    return -1;
  }
  free(configStr);

  int width  = atoi((const char*) hash_map_get_value(&config, CONFIG_WIDTH));
  int height = atoi((const char*) hash_map_get_value(&config, CONFIG_HEIGHT));
  char* host = hash_map_get_value(&config, CONFIG_HOST);
  char* port = hash_map_get_value(&config, CONFIG_PORT);

  HWND window = game_window_create(width, height, WM_WINDOWED);
  RenderInstance* renderInstance = render_instance_create(window);
  if (renderInstance == NULL)
  {
    fprintf(stderr, "Failed to initialize render instance.\n");
    hash_map_destroy(&config, free);
    game_window_destroy(window);
    profiler_destroy();
    return -1;
  }

  // TODO: make this better.
  // ------
  const float baseComp        = 1.0f / sqrtf(3.0f);
  const MeshVertex vertices[] = {
      {.position  = {-0.5f, -0.5f, 0.5f},
          .normal = {-baseComp, -baseComp, baseComp}},
      {.position  = {0.5f, -0.5f, 0.5f},
          .normal = {baseComp, -baseComp, baseComp}},
      {.position  = {-0.5f, 0.5f, 0.5f},
          .normal = {-baseComp, baseComp, baseComp}},
      {.position  = {0.5f, 0.5f, 0.5f},
          .normal = {baseComp, baseComp, baseComp}},
      {.position  = {-0.5f, -0.5f, -0.5f},
          .normal = {-baseComp, -baseComp, -baseComp}},
      {.position  = {0.5f, -0.5f, -0.5f},
          .normal = {baseComp, -baseComp, -baseComp}},
      {.position  = {-0.5f, 0.5f, -0.5f},
          .normal = {-baseComp, baseComp, -baseComp}},
      {.position  = {0.5f, 0.5f, -0.5f},
          .normal = {baseComp, baseComp, -baseComp}}};
  const uint16_t indices[] = {
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

  // TODO: Move copy command to outside of mesh create to allow for batching.
  Mesh* cube = mesh_create(vertices, sizeof(MeshVertex), _countof(vertices),
      indices, _countof(indices), renderInstance->physicalDevice,
      renderInstance->logicalDevice, renderInstance->commandPool,
      graphicsQueue);

  AutoArray buildingMesh;
  auto_array_create(&buildingMesh, sizeof(Mesh*));
  for (uint32_t i = 0; i < asset.meshes.size; i++)
  {
    Mesh** mesh             = auto_array_allocate(&buildingMesh);
    GlbAssetMesh* assetMesh = auto_array_get(&asset.meshes, i);
    *mesh = mesh_create(assetMesh->vertices, sizeof(MeshVertex),
        assetMesh->numOfVertices, assetMesh->indices, assetMesh->numOfIndices,
        renderInstance->physicalDevice, renderInstance->logicalDevice,
        renderInstance->commandPool, graphicsQueue);
  }
  // ------

  UdpGameClient client;
  if (!udp_game_client_connect(&client, host, port))
  {
    MessageBox(window, L"Unable to connect to server.", L"Connection Issue!",
        MB_ICONERROR);
    hash_map_destroy(&config, free);
    render_instance_destroy(renderInstance);
    game_window_destroy(window);
    profiler_destroy();
    return -1;
  }

  Connection connection = {.state = CS_NOT_JOINED,
      .lastInput                  = {0},
      .lastHeartbeatTime          = 0,
      .lastStateTime              = 0};

  QueryPerformanceCounter(&connection.lastHeartbeatTime);
  connection.lastStateTime    = connection.lastHeartbeatTime;
  LARGE_INTEGER lastFrameTime = connection.lastStateTime;
  LARGE_INTEGER lastStatTime  = connection.lastStateTime;

  renderInstance->cameraPosition.x = 100.0f;
  renderInstance->cameraPosition.y = 4.0f;
  renderInstance->cameraPosition.z = 100.0f;
  while (!game_window_process_message())
  {
    handle_connection(&connection, &client);

    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    float deltaTime = ((float) (currentTime.QuadPart - lastFrameTime.QuadPart)
                       / g_timerFrequency.QuadPart);
    game_state_update(NULL, deltaTime);

    // Draw scene
    Transform floorTransform;
    transform_identity(&floorTransform);
    floorTransform.position.y = 1.0f;
    floorTransform.scale.x    = 100.0f;
    floorTransform.scale.z    = 100.0f;
    render_instance_queue_mesh_draw(cube, &floorTransform, renderInstance);

    for (uint32_t i = 0; i < buildingMesh.size; i++)
    {
      // TODO: Properly package assets
      Mesh** assetMesh       = auto_array_get(&buildingMesh, i);
      GlbAssetMesh* glbAsset = auto_array_get(&asset.meshes, i);
      render_instance_queue_mesh_draw(
          *assetMesh, &glbAsset->transform, renderInstance);
    }

    // Draw players
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
      if (g_listOfPlayers[i].active)
      {
        if (memcmp(&g_listOfPlayers[i].id, &g_clientGuid, sizeof(GUID)) == 0)
        {
          renderInstance->cameraPosition.x = g_listOfPlayers[i].positionX;
          renderInstance->cameraPosition.y = 1.0f;
          renderInstance->cameraPosition.z = g_listOfPlayers[i].positionY;
        }
        else
        {
          // TODO: Make list of players use transform.
          Transform playerTransform;
          transform_identity(&playerTransform);
          playerTransform.position.x = g_listOfPlayers[i].positionX;
          playerTransform.position.y = 0.0;
          playerTransform.position.z = g_listOfPlayers[i].positionY;
          render_instance_queue_mesh_draw(
              cube, &playerTransform, renderInstance);
        }
      }
    }

    render_instance_draw(renderInstance);

    // TODO: Make a timer utility. THis is just getting ridiculous.
    if ((float) (currentTime.QuadPart - lastStatTime.QuadPart)
            / g_timerFrequency.QuadPart
        > 1.0f)
    {
      const char* keys[] = {"bvh_init", "bvh_build", "bvh_create",
          "cpu_shadow_rt", "cpu_buffer_copy", "render_submit"};
      for (int i = 0; i < _countof(keys); i++)
      {
        float time = profiler_clock_get(keys[i]);
        printf("[%s]        \t%f ms\n", keys[i], time * 1000.0f);
      }
      lastStatTime = currentTime;
    }

    lastFrameTime = currentTime;
  }

  if (connection.state == CS_JOINED)
  {
    Message* disconnectMessage = message_create_leave_request(&g_clientGuid);
    if (disconnectMessage != NULL)
    {
      udp_game_client_send_message(&client, disconnectMessage);
      message_destroy(disconnectMessage);
      profiler_destroy();
    }
  }

  mesh_destroy(cube, renderInstance->logicalDevice);
  udp_game_client_destroy(&client);
  hash_map_destroy(&config, free);
  render_instance_destroy(renderInstance);
  game_window_destroy(window);
  profiler_destroy();

  return 0;
}
