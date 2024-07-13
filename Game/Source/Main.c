#include "Config/GameConfig.h"
#include "Input/InputMap.h"
#include "Otter/Async/Scheduler.h"
#include "Otter/Math/Mat.h"
#include "Otter/Math/Transform.h"
#include "Otter/Math/Vec.h"
#include "Otter/Render/Gltf/GlbAsset.h"
#include "Otter/Render/Material.h"
#include "Otter/Render/Mesh.h"
#include "Otter/Render/RenderInstance.h"
#include "Otter/Render/Texture/Texture.h"
#include "Otter/Util/File.h"
#include "Otter/Util/Log.h"
#include "Otter/Util/Profiler.h"
#include "Window/GameWindow.h"

static LARGE_INTEGER g_timerFrequency;

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
  const float SPEED              = 50.0f;

  float moveForward  = input_map_get_action_value(map, "move_forward");
  float moveBackward = input_map_get_action_value(map, "move_back");
  if (!isnan(moveForward) && !isnan(moveBackward))
  {
    Vec4 translation = {0.0f, 0.0f, 1.0f, 1.0f};
    Mat4 rotationMatrix;
    mat4_identity(rotationMatrix);
    mat4_rotate(rotationMatrix, renderInstance->cameraTransform.rotation.x,
        renderInstance->cameraTransform.rotation.y,
        renderInstance->cameraTransform.rotation.z);
    vec4_multiply_mat4(&translation, rotationMatrix);

    Vec3* translation3d = (Vec3*) &translation;
    vec3_multiply(
        translation3d, -(moveForward - moveBackward) * SPEED * deltaTime);
    vec3_add(&renderInstance->cameraTransform.position, (Vec3*) &translation);
  }

  float moveRight = input_map_get_action_value(map, "move_right");
  float moveLeft  = input_map_get_action_value(map, "move_left");
  if (!isnan(moveRight) && !isnan(moveLeft))
  {
    Vec4 translation = {1.0f, 0.0f, 0.0f, 1.0f};
    Mat4 rotationMatrix;
    mat4_identity(rotationMatrix);
    mat4_rotate(rotationMatrix, renderInstance->cameraTransform.rotation.x,
        renderInstance->cameraTransform.rotation.y,
        renderInstance->cameraTransform.rotation.z);
    vec4_multiply_mat4(&translation, rotationMatrix);

    Vec3* translation3d = (Vec3*) &translation;
    vec3_multiply(translation3d, -(moveLeft - moveRight) * SPEED * deltaTime);
    vec3_add(&renderInstance->cameraTransform.position, (Vec3*) &translation);
  }

  float turnLeft  = input_map_get_action_value(map, "turn_left");
  float turnRight = input_map_get_action_value(map, "turn_right");
  if (!isnan(turnLeft) && !isnan(turnRight))
  {
    renderInstance->cameraTransform.rotation.y +=
        (turnLeft - turnRight) * deltaTime * 3.f;
  }

  float moveUp   = input_map_get_action_value(map, "move_up");
  float moveDown = input_map_get_action_value(map, "move_down");
  if (!isnan(moveUp) && !isnan(moveDown))
  {
    renderInstance->cameraTransform.position.y -=
        (moveUp - moveDown) * deltaTime * 3.f;
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

int WINAPI wWinMain(
    HINSTANCE instance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow)
{
  (void) instance;
  (void) prevInstance;
  (void) cmdLine;
  (void) cmdShow;

  QueryPerformanceFrequency(&g_timerFrequency);

  GameConfig config;
  if (!game_config_parse(&config, DEFAULT_GAME_CONFIG_PATH))
  {
    return -1;
  }

  size_t fileLength = 0;
  char* glbTest     = file_load(config.sampleModel, &fileLength);
  if (glbTest == NULL)
  {
    LOG_ERROR("Unable to find file %s", config.sampleModel);
    game_config_destroy(&config);
    return -1;
  }

  GlbAsset asset;
  if (!glb_load_asset(glbTest, fileLength, &asset))
  {
    LOG_ERROR("Unable to parse %s", config.sampleModel);
    free(glbTest);
    game_config_destroy(&config);
    return -1;
  }
  free(glbTest);

  task_scheduler_init();
  profiler_init(g_timerFrequency);
  HWND window = game_window_create(config.width, config.height, WM_WINDOWED);
  RenderInstance* renderInstance =
      render_instance_create(window, config.shaderDirectory);
  if (renderInstance == NULL)
  {
    LOG_ERROR("Failed to initialize render instance.");
    game_config_destroy(&config);
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
      1, 2, 0, 1, 3, 2, // Front
      5, 3, 1, 5, 7, 3, // Right
      4, 7, 5, 4, 6, 7, // Back
      0, 6, 4, 0, 2, 6, // Left
      1, 0, 4, 5, 1, 4, // Top
      3, 6, 2, 3, 7, 6 // Bottom
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

  AutoArray textures;
  auto_array_create(&textures, sizeof(Texture));
  for (uint32_t i = 0; i < asset.images.size; i++)
  {
    GlbAssetImage* assetTexture = auto_array_get(&asset.images, i);

    Texture* sampler = auto_array_allocate(&textures);
    if (!texture_create(sampler, assetTexture->data, assetTexture->width,
            assetTexture->height, assetTexture->channels,
            renderInstance->physicalDevice, renderInstance->logicalDevice,
            renderInstance->commandPool, graphicsQueue))
    {
      LOG_ERROR("Failed to create texture.");
      auto_array_destroy(&buildingMesh);
      game_config_destroy(&config);
      task_scheduler_destroy();
      mesh_destroy(cube, renderInstance->logicalDevice);
      render_instance_destroy(renderInstance);
      game_window_destroy(window);
      profiler_destroy();
      return -1;
    }
  }
  // ------

  LARGE_INTEGER lastFrameTime;
  QueryPerformanceCounter(&lastFrameTime);
  LARGE_INTEGER lastStatTime = lastFrameTime;

  InputMap inputMap;
  if (!input_map_create(&inputMap))
  {
    game_config_destroy(&config);
    render_instance_destroy(renderInstance);
    game_window_destroy(window);
    profiler_destroy();
    task_scheduler_destroy();
    return -1;
  }

  input_map_load_key_binds_from_file(&inputMap, DEFAULT_KEY_BINDS_PATH);

  renderInstance->cameraTransform.position.x = 0.0f;
  renderInstance->cameraTransform.position.y = -4.0f;
  renderInstance->cameraTransform.position.z = 100.0f;

  Texture defaultTexture;
  uint8_t defaultTextureData[] = {0, 0, 255, 255};
  if (!texture_create(&defaultTexture, defaultTextureData, 1, 1, 4,
          renderInstance->physicalDevice, renderInstance->logicalDevice,
          renderInstance->commandPool, graphicsQueue))
  {
    LOG_ERROR("Failed to create default texture.");
    auto_array_destroy(&buildingMesh);
    game_config_destroy(&config);
    input_map_destroy(&inputMap);
    task_scheduler_destroy();
    mesh_destroy(cube, renderInstance->logicalDevice);
    render_instance_destroy(renderInstance);
    game_window_destroy(window);
    profiler_destroy();
    return -1;
  }

  while (!game_window_process_message(window))
  {
    profiler_clock_start("preframe");
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    float deltaTime = ((float) (currentTime.QuadPart - lastFrameTime.QuadPart)
                       / g_timerFrequency.QuadPart);

    AutoArray* inputs = (AutoArray*) GetWindowLongPtr(window, GWLP_USERDATA);
    input_map_update(&inputMap, inputs, deltaTime);

    pseudoOnUpdate(&inputMap, renderInstance, deltaTime);

    // Draw scene
    Mat4 floorTransform;
    mat4_identity(floorTransform);
    mat4_translate(floorTransform, 0.0f, 10.0f, 0.0f);
    mat4_scale(floorTransform, 100.0f, 1.0f, 100.0f);
    render_instance_queue_mesh_draw(
        cube, floorTransform, &defaultTexture.sampler, renderInstance);

    // Draw light source
    Mat4 lightTransform;
    mat4_identity(lightTransform);
    mat4_translate(lightTransform, 16.0f, -16.0f, 16.0f);
    render_instance_queue_mesh_draw(
        cube, lightTransform, &defaultTexture.sampler, renderInstance);

    for (uint32_t i = 0; i < buildingMesh.size; i++)
    {
      // TODO: Properly package assets
      Mesh** assetMesh       = auto_array_get(&buildingMesh, i);
      GlbAssetMesh* glbAsset = auto_array_get(&asset.meshes, i);
      GlbAssetMaterial* material =
          auto_array_get(&asset.materials, glbAsset->materialIndex);
      ImageSampler* sampler = &defaultTexture.sampler;
      if (material->baseColorTexture < textures.size)
      {
        uint32_t* textureIndex =
            auto_array_get(&asset.textures, material->baseColorTexture);
        Texture* texture = auto_array_get(&textures, *textureIndex);
        sampler          = &texture->sampler;
      }
      render_instance_queue_mesh_draw(
          *assetMesh, glbAsset->transform, sampler, renderInstance);
    }
    profiler_clock_end("preframe");

    render_instance_draw(renderInstance);

    // TODO: Make a timer utility. THis is just getting ridiculous.
    if ((float) (currentTime.QuadPart - lastStatTime.QuadPart)
            / g_timerFrequency.QuadPart
        > 1.0f)
    {
      const char* keys[] = {};
      for (int i = 0; i < _countof(keys); i++)
      {
        float time = profiler_clock_get(keys[i]);
        printf("[%s]        \t%f ms\n", keys[i], time * 1000.0f);
      }
      lastStatTime = currentTime;
    }

    lastFrameTime = currentTime;
  }

  render_instance_wait_for_idle(renderInstance);

  for (uint32_t i = 0; i < buildingMesh.size; i++)
  {
    // TODO: Properly package assets
    Mesh** assetMesh = auto_array_get(&buildingMesh, i);
    mesh_destroy(*assetMesh, renderInstance->logicalDevice);
  }

  for (uint32_t i = 0; i < textures.size; i++)
  {
    Texture* texture = auto_array_get(&textures, i);
    texture_destroy(texture, renderInstance->logicalDevice);
  }
  texture_destroy(&defaultTexture, renderInstance->logicalDevice);
  auto_array_destroy(&buildingMesh);
  game_config_destroy(&config);
  input_map_destroy(&inputMap);
  task_scheduler_destroy();
  mesh_destroy(cube, renderInstance->logicalDevice);
  render_instance_destroy(renderInstance);
  game_window_destroy(window);
  profiler_destroy();

  return 0;
}

