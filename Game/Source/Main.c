#include "Input/InputMap.h"
#include "Otter/Async/Scheduler.h"
#include "Otter/Config/Config.h"
#include "Otter/Math/Transform.h"
#include "Otter/Math/Vec.h"
#include "Otter/Render/Gltf/GlbAsset.h"
#include "Otter/Render/Mesh.h"
#include "Otter/Render/RenderInstance.h"
#include "Otter/Util/File.h"
#include "Otter/Util/HashMap.h"
#include "Otter/Util/Log.h"
#include "Otter/Util/Profiler.h"
#include "Window/GameWindow.h"

#define CONFIG_WIDTH  "width"
#define CONFIG_HEIGHT "height"

static LARGE_INTEGER g_timerFrequency;

typedef struct GameConfig
{
  int width;
  int height;
} GameConfig;

static bool loadGameConfig(GameConfig* config)
{
  char* configStr = file_load("Config/client.ini", NULL);
  if (configStr == NULL)
  {
    LOG_ERROR("Unable to find file config.ini");
    return false;
  }

  HashMap configMap;
  if (!config_parse(&configMap, configStr))
  {
    free(configStr);
    LOG_ERROR("Could not parse configuration.");
    return false;
  }
  free(configStr);

  char* widthStr =
      hash_map_get_value(&configMap, CONFIG_WIDTH, strlen(CONFIG_WIDTH) + 1);
  char* heightStr =
      hash_map_get_value(&configMap, CONFIG_HEIGHT, strlen(CONFIG_HEIGHT) + 1);
  config->width  = widthStr != NULL ? atoi(widthStr) : 1920;
  config->height = heightStr != NULL ? atoi(heightStr) : 1080;
  LOG_DEBUG("Setting window to (%d, %d)", config->width, config->height);

  hash_map_destroy(&configMap, free);

  return true;
}

int main()
{
  wWinMain(GetModuleHandle(NULL), NULL, L"", 1);
}

static void pseudoOnUpdate(
    InputMap* map, RenderInstance* renderInstance, float deltaTime)
{
  static float timer             = 0.0f;
  static uint16_t rumbleStrength = 0;
  static bool pressed            = false;
  const float SPEED              = 200.0f;

  float moveForward  = input_map_get_action_value(map, "move_forward");
  float moveBackward = input_map_get_action_value(map, "move_back");
  if (!isnan(moveForward) && !isnan(moveBackward))
  {
    renderInstance->cameraPosition.z +=
        (moveBackward - moveForward) * deltaTime * SPEED;
  }

  float moveRight = input_map_get_action_value(map, "move_right");
  float moveLeft  = input_map_get_action_value(map, "move_left");
  if (!isnan(moveRight) && !isnan(moveLeft))
  {
    renderInstance->cameraPosition.x +=
        (moveRight - moveLeft) * deltaTime * SPEED;
  }

  float switchSpeed = input_map_get_action_value(map, "set_speed");
  if (!isnan(switchSpeed))
  {
    if (switchSpeed > 0.0f && !pressed)
    {
      rumbleStrength += 1;
      rumbleStrength %= 4;
      pressed = true;
      LOG_DEBUG("Rumble strength: %d", rumbleStrength);
    }
    else if (switchSpeed <= 0.0f)
    {
      pressed = false;
    }
  }

  timer += deltaTime;
  if (timer > 5.0f)
  {
    switch (rumbleStrength)
    {
    case 1:
      input_map_queue_rumble_effect(map, 0, RP_HIGH_FREQUENCY, 1000, 3.0f);
      input_map_queue_rumble_effect(map, 0, RP_HIGH_FREQUENCY, 63000, 1.0f);
      break;
    case 2:
      input_map_queue_rumble_effect(map, 0, RP_LOW_FREQUENCY, 1000, 3.0f);
      input_map_queue_rumble_effect(map, 0, RP_LOW_FREQUENCY, 63000, 1.0f);
      break;
    case 3:
      input_map_queue_rumble_effect(map, 0, RP_HIGH_FREQUENCY, 1000, 3.0f);
      input_map_queue_rumble_effect(map, 0, RP_HIGH_FREQUENCY, 63000, 1.0f);
      input_map_queue_rumble_effect(map, 0, RP_LOW_FREQUENCY, 1000, 3.0f);
      input_map_queue_rumble_effect(map, 0, RP_LOW_FREQUENCY, 63000, 1.0f);
    default:
      break;
    }
    timer = 0.0f;
  }
}

static bool load_key_binds(HashMap* keyBinds, const char* path)
{
  char* keyBindStr = file_load(path, NULL);
  if (keyBindStr == NULL)
  {
    LOG_ERROR("Unable to find file %s", path);
    return false;
  }

  if (!config_parse(keyBinds, keyBindStr))
  {
    LOG_ERROR("Could not parse key binds.");
    free(keyBindStr);
    return false;
  }

  free(keyBindStr);

  return true;
}

int WINAPI wWinMain(
    HINSTANCE instance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow)
{
  (void) instance;
  (void) prevInstance;
  (void) cmdLine;
  (void) cmdShow;

  QueryPerformanceFrequency(&g_timerFrequency);

  size_t fileLength = 0;
  char* glbTest     = file_load("model.glb", &fileLength);
  if (glbTest == NULL)
  {
    LOG_ERROR("Unable to find file model.glb");
    return -1;
  }

  GlbAsset asset;
  if (!glb_load_asset(glbTest, fileLength, &asset))
  {
    LOG_ERROR("Unable to parse model.glb");
    return -1;
  }
  free(glbTest);

  GameConfig config;
  if (!loadGameConfig(&config))
  {
    return -1;
  }

  task_scheduler_init();
  profiler_init(g_timerFrequency);
  HWND window = game_window_create(config.width, config.height, WM_WINDOWED);
  RenderInstance* renderInstance = render_instance_create(window);
  if (renderInstance == NULL)
  {
    LOG_ERROR("Failed to initialize render instance.");
    game_window_destroy(window);
    profiler_destroy();
    task_scheduler_destroy();
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
  // Mesh* cube = mesh_create(vertices, sizeof(MeshVertex), _countof(vertices),
  //    indices, _countof(indices), renderInstance->physicalDevice,
  //    renderInstance->logicalDevice, renderInstance->commandPool,
  //    graphicsQueue);

  // AutoArray buildingMesh;
  // auto_array_create(&buildingMesh, sizeof(Mesh*));
  // for (uint32_t i = 0; i < asset.meshes.size; i++)
  //{
  //   Mesh** mesh             = auto_array_allocate(&buildingMesh);
  //   GlbAssetMesh* assetMesh = auto_array_get(&asset.meshes, i);
  //   *mesh = mesh_create(assetMesh->vertices, sizeof(MeshVertex),
  //       assetMesh->numOfVertices, assetMesh->indices,
  //       assetMesh->numOfIndices, renderInstance->physicalDevice,
  //       renderInstance->logicalDevice, renderInstance->commandPool,
  //       graphicsQueue);
  // }
  //  ------

  LARGE_INTEGER lastFrameTime;
  QueryPerformanceCounter(&lastFrameTime);
  LARGE_INTEGER lastStatTime = lastFrameTime;

  InputMap inputMap;
  if (!input_map_create(&inputMap))
  {
    render_instance_destroy(renderInstance);
    game_window_destroy(window);
    profiler_destroy();
    task_scheduler_destroy();
    return -1;
  }

  HashMap keyBinds;
  if (!load_key_binds(&keyBinds, "Config/keybinds.ini"))
  {
    render_instance_destroy(renderInstance);
    game_window_destroy(window);
    profiler_destroy();
    task_scheduler_destroy();
    return -1;
  }
  input_map_load_key_binds(&inputMap, &keyBinds);
  hash_map_destroy(&keyBinds, free);

  renderInstance->cameraPosition.x = 100.0f;
  renderInstance->cameraPosition.y = 4.0f;
  renderInstance->cameraPosition.z = 100.0f;

  while (!game_window_process_message(window))
  {
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    float deltaTime = ((float) (currentTime.QuadPart - lastFrameTime.QuadPart)
                       / g_timerFrequency.QuadPart);

    AutoArray* inputs = (AutoArray*) GetWindowLongPtr(window, GWLP_USERDATA);
    input_map_update(&inputMap, inputs, deltaTime);

    pseudoOnUpdate(&inputMap, renderInstance, deltaTime);

    // Draw scene
    Transform floorTransform;
    transform_identity(&floorTransform);
    floorTransform.position.y = 10.0f;
    floorTransform.scale.x    = 100.0f;
    floorTransform.scale.z    = 100.0f;
    // render_instance_queue_mesh_draw(cube, &floorTransform, renderInstance);

    // for (uint32_t i = 0; i < buildingMesh.size; i++)
    //{
    //   // TODO: Properly package assets
    //   Mesh** assetMesh       = auto_array_get(&buildingMesh, i);
    //   GlbAssetMesh* glbAsset = auto_array_get(&asset.meshes, i);
    //   render_instance_queue_mesh_draw(
    //       *assetMesh, &glbAsset->transform, renderInstance);
    // }

    render_instance_draw(renderInstance);

    // TODO: Make a timer utility. THis is just getting ridiculous.
    if ((float) (currentTime.QuadPart - lastStatTime.QuadPart)
            / g_timerFrequency.QuadPart
        > 1.0f)
    {
      // Read profile keys from config file and overlay them with imgui.
      // const char* keys[] = {"bvh_init", "bvh_build", "bvh_create",
      //     "cpu_shadow_rt", "cpu_buffer_copy", "render_submit"};
      // for (int i = 0; i < _countof(keys); i++)
      // {
      //   float time = profiler_clock_get(keys[i]);
      //   printf("[%s]        \t%f ms\n", keys[i], time * 1000.0f);
      // }
      lastStatTime = currentTime;
    }

    lastFrameTime = currentTime;
  }

  input_map_destroy(&inputMap);
  task_scheduler_destroy();
  // mesh_destroy(cube, renderInstance->logicalDevice);
  render_instance_destroy(renderInstance);
  game_window_destroy(window);
  profiler_destroy();

  return 0;
}

