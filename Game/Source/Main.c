#include "Components/ComponentType.h"
#include "Config/GameConfig.h"
#include "Input/InputMap.h"
#include "Otter/Async/Scheduler.h"
#include "Otter/ECS/EntityComponentMap.h"
#include "Otter/ECS/SystemRegistry.h"
#include "Otter/Math/Mat.h"
#include "Otter/Math/Transform.h"
#include "Otter/Math/Vec.h"
#include "Otter/Render/Gltf/GlbAsset.h"
#include "Otter/Render/Mesh.h"
#include "Otter/Render/RenderInstance.h"
#include "Otter/Render/Texture/Texture.h"
#include "Otter/Util/File.h"
#include "Otter/Util/Log.h"
#include "Otter/Util/Profiler.h"
#include "Window/GameWindow.h"

typedef struct Context
{
  InputMap* inputMap;
  RenderInstance* renderInstance;
  float deltaTime;
} Context;

static LARGE_INTEGER g_timerFrequency;

int main()
{
  wWinMain(GetModuleHandle(NULL), NULL, L"", 1);
}

static void update_camera_position(
    InputMap* map, RenderInstance* renderInstance, float deltaTime)
{
  const float SPEED = 50.0f;

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
}

static void update_position(
    Context* context, uint64_t entity, void** components)
{
  Mat4* position = (Mat4*) components[0];
  Vec3* velocity = (Vec3*) components[1];

  if ((*position)[3][0] > 16.0f)
  {
    velocity->x = -1.0f;
  }
  else if ((*position)[3][0] < -16.0f)
  {
    velocity->x = 1.0f;
  }

  Vec3 v = *velocity;
  vec3_multiply(&v, context->deltaTime * 10.0f);
  mat4_translate(*position, v.x, v.y, v.z);
}

static void render_mesh_system(
    Context* context, uint64_t entity, void** components)
{
  Mat4* transform     = (Mat4*) components[0];
  Mesh** mesh         = (Mesh**) components[1];
  Material** material = (Material**) components[2];

  render_instance_queue_mesh_draw(
      *mesh, *material, *transform, context->renderInstance);
}

int WINAPI wWinMain(
    HINSTANCE instance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow)
{
  (void) instance;
  (void) prevInstance;
  (void) cmdLine;
  (void) cmdShow;

  QueryPerformanceFrequency(&g_timerFrequency);
  task_scheduler_init();

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
  Mesh cubeMesh;
  if (!mesh_create(&cubeMesh, vertices, sizeof(MeshVertex), _countof(vertices),
          indices, _countof(indices), renderInstance->physicalDevice,
          renderInstance->logicalDevice, renderInstance->commandPool,
          graphicsQueue))
  {
    LOG_ERROR("Failed to create cube mesh.");
    game_config_destroy(&config);
    task_scheduler_destroy();
    render_instance_destroy(renderInstance);
    game_window_destroy(window);
    profiler_destroy();
    return -1;
  }

  Texture defaultTexture;
  uint8_t defaultTextureData[] = {0, 0, 255, 255};
  if (!texture_create(&defaultTexture, defaultTextureData, 1, 1, 4, TT_COLOR,
          true, renderInstance->physicalDevice, renderInstance->logicalDevice,
          renderInstance->commandPool, graphicsQueue))
  {
    LOG_ERROR("Failed to create default texture.");
    game_config_destroy(&config);
    task_scheduler_destroy();
    mesh_destroy(&cubeMesh, renderInstance->logicalDevice);
    render_instance_destroy(renderInstance);
    game_window_destroy(window);
    profiler_destroy();
    return -1;
  }

  Material defaultMaterial = {
      .constant                 = {.baseColorFactor    = {1.0f, 1.0f, 1.0f, 1.0f},
                          .useBaseColorTexture         = false,
                          .metallicFactor              = 0.0f,
                          .roughnessFactor             = 1.0f,
                          .useMetallicRoughnessTexture = false,
                          .occlusionStrength           = 0.0f,
                          .useOcclusionTexture         = false},
      .baseColorTexture         = &defaultTexture.sampler,
      .normalTexture            = &defaultTexture.sampler,
      .metallicRoughnessTexture = &defaultTexture.sampler,
      .occlusionTexture         = &defaultTexture.sampler};

  AutoArray textureImages;
  LOG_DEBUG("Loading textures to GPU");
  auto_array_create(&textureImages, sizeof(Texture));
  auto_array_allocate_many(&textureImages, asset.images.size);
  for (uint32_t i = 0; i < asset.images.size; i++)
  {
    GlbAssetImage* assetTexture = auto_array_get(&asset.images, i);

    Texture* sampler = auto_array_get(&textureImages, i);
    // TODO: This creates way too many buffers via the transfer buffer. Maybe
    // have a scratch buffer for large loads
    if (!texture_create(sampler, assetTexture->data, assetTexture->width,
            assetTexture->height, assetTexture->channels,
            assetTexture->colorType == GICT_SRGB ? TT_COLOR : TT_NONCOLOR, true,
            renderInstance->physicalDevice, renderInstance->logicalDevice,
            renderInstance->commandPool, graphicsQueue))
    {
      LOG_ERROR("Failed to create texture.");
      game_config_destroy(&config);
      task_scheduler_destroy();
      mesh_destroy(&cubeMesh, renderInstance->logicalDevice);
      render_instance_destroy(renderInstance);
      game_window_destroy(window);
      profiler_destroy();
      return -1;
    }
  }

  StableAutoArray materials;
  stable_auto_array_create(
      &materials, sizeof(Material), SAA_DEFAULT_CHUNK_SIZE);
  for (size_t i = 0; i < asset.materials.size; i++)
  {
    GlbAssetMaterial* assetMaterial = auto_array_get(&asset.materials, i);

    Material* material                 = stable_auto_array_allocate(&materials);
    material->constant.baseColorFactor = assetMaterial->baseColor;
    material->constant.metallicFactor  = assetMaterial->metallicFactor;
    material->constant.roughnessFactor = assetMaterial->roughnessFactor;
    material->constant.occlusionStrength   = assetMaterial->occlusionStrength;
    material->constant.useBaseColorTexture = VK_FALSE;
    material->constant.useNormalTexture    = VK_FALSE;
    material->constant.useMetallicRoughnessTexture = VK_FALSE;
    material->constant.useOcclusionTexture         = VK_FALSE;
    material->baseColorTexture                     = &defaultTexture.sampler;
    material->normalTexture                        = &defaultTexture.sampler;
    material->metallicRoughnessTexture             = &defaultTexture.sampler;
    material->occlusionTexture                     = &defaultTexture.sampler;
    material->alphaMode                            = assetMaterial->alphaMode;
    material->alphaCutoff                          = assetMaterial->alphaCutoff;

    if (assetMaterial->baseColorTexture < asset.textures.size)
    {
      uint32_t* textureIndex =
          auto_array_get(&asset.textures, assetMaterial->baseColorTexture);
      Texture* texture = auto_array_get(&textureImages, *textureIndex);
      material->baseColorTexture             = &texture->sampler;
      material->constant.useBaseColorTexture = VK_TRUE;
    }

    if (assetMaterial->normalTexture < asset.textures.size)
    {
      uint32_t* textureIndex =
          auto_array_get(&asset.textures, assetMaterial->normalTexture);
      Texture* texture        = auto_array_get(&textureImages, *textureIndex);
      material->normalTexture = &texture->sampler;
      material->constant.useNormalTexture = VK_TRUE;
    }

    if (assetMaterial->metallicRoughnessTexture < asset.textures.size)
    {
      uint32_t* textureIndex = auto_array_get(
          &asset.textures, assetMaterial->metallicRoughnessTexture);
      Texture* texture = auto_array_get(&textureImages, *textureIndex);
      material->metallicRoughnessTexture             = &texture->sampler;
      material->constant.useMetallicRoughnessTexture = VK_TRUE;
    }

    if (assetMaterial->occlusionTexture < asset.textures.size)
    {
      uint32_t* textureIndex =
          auto_array_get(&asset.textures, assetMaterial->occlusionTexture);
      Texture* texture = auto_array_get(&textureImages, *textureIndex);
      material->occlusionTexture             = &texture->sampler;
      material->constant.useOcclusionTexture = VK_TRUE;
    }
  }

  AutoArray buildingMesh;
  auto_array_create(&buildingMesh, sizeof(Mesh));
  LOG_DEBUG("Loading meshes to GPU");
  for (uint32_t i = 0; i < asset.meshes.size; i++)
  {

    Mesh* mesh              = auto_array_allocate(&buildingMesh);
    GlbAssetMesh* assetMesh = auto_array_get(&asset.meshes, i);
    if (!mesh_create(mesh, assetMesh->vertices, sizeof(MeshVertex),
            assetMesh->numOfVertices, assetMesh->indices,
            assetMesh->numOfIndices, renderInstance->physicalDevice,
            renderInstance->logicalDevice, renderInstance->commandPool,
            graphicsQueue))
    {
      LOG_ERROR("Failed to create mesh.");
      game_config_destroy(&config);
      task_scheduler_destroy();
      mesh_destroy(&cubeMesh, renderInstance->logicalDevice);
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

  EntityComponentMap entityComponentMap;
  entity_component_map_create(&entityComponentMap);
  component_pool_register_component(
      &entityComponentMap.componentPool, CT_TRANSFORM, sizeof(Mat4));
  component_pool_register_component(
      &entityComponentMap.componentPool, CT_MESH, sizeof(Mesh*));
  component_pool_register_component(
      &entityComponentMap.componentPool, CT_MATERIAL, sizeof(Material*));
  component_pool_register_component(
      &entityComponentMap.componentPool, CT_VELOCITY, sizeof(Vec3));

  SystemRegistry systemRegistry;
  system_registry_create(&systemRegistry);
  system_registry_register_system(&systemRegistry,
      (SystemCallback) render_mesh_system, 3, CT_TRANSFORM, CT_MESH,
      CT_MATERIAL);
  system_registry_register_system(&systemRegistry,
      (SystemCallback) update_position, 2, CT_TRANSFORM, CT_VELOCITY);

  // Create the light.
  // TODO: We probably want to make a parenting system for entities.
  uint64_t light = entity_component_map_create_entity(&entityComponentMap);

  entity_component_map_add_component(&entityComponentMap, light, CT_TRANSFORM);
  Mat4* transform = (Mat4*) entity_component_map_get_component(
      &entityComponentMap, light, CT_TRANSFORM);
  mat4_identity(*transform);
  mat4_translate(*transform, 16.0f, -16.0f, 16.0f);

  entity_component_map_add_component(&entityComponentMap, light, CT_VELOCITY);
  Vec3* velocity = (Vec3*) entity_component_map_get_component(
      &entityComponentMap, light, CT_VELOCITY);
  *velocity = (Vec3){1.0f, 0.0f, 0.0f};

  entity_component_map_add_component(&entityComponentMap, light, CT_MESH);
  Mesh** mesh = (Mesh**) entity_component_map_get_component(
      &entityComponentMap, light, CT_MESH);
  *mesh = &cubeMesh;

  entity_component_map_add_component(&entityComponentMap, light, CT_MATERIAL);
  Material** material = (Material**) entity_component_map_get_component(
      &entityComponentMap, light, CT_MATERIAL);
  *material = &defaultMaterial;

  InputMap inputMap;
  if (!input_map_create(&inputMap))
  {
    LOG_ERROR("Failed to create input map.");
    system_registry_destroy(&systemRegistry);
    entity_component_map_destroy(&entityComponentMap);
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

  Context context = {
      .inputMap = &inputMap, .renderInstance = renderInstance, .deltaTime = 0};
  while (!game_window_process_message(window))
  {
    profiler_clock_start("preframe");
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    context.deltaTime = ((float) (currentTime.QuadPart - lastFrameTime.QuadPart)
                         / g_timerFrequency.QuadPart);

    AutoArray* inputs = (AutoArray*) GetWindowLongPtr(window, GWLP_USERDATA);
    input_map_update(&inputMap, inputs, context.deltaTime);

    update_camera_position(&inputMap, renderInstance, context.deltaTime);

    system_registry_run_systems(&systemRegistry, &entityComponentMap, &context);

    // Draw scene
    Mat4 floorTransform;
    mat4_identity(floorTransform);
    mat4_translate(floorTransform, 0.0f, 10.0f, 0.0f);
    mat4_scale(floorTransform, 100.0f, 1.0f, 100.0f);
    render_instance_queue_mesh_draw(
        &cubeMesh, &defaultMaterial, floorTransform, renderInstance);

    for (uint32_t i = 0; i < buildingMesh.size; i++)
    {
      // TODO: Properly package assets
      Mesh* assetMesh = auto_array_get(&buildingMesh, i);

      // TODO: Remap this to use only our representation of materials
      GlbAssetMesh* glbAsset = auto_array_get(&asset.meshes, i);
      GlbAssetMaterial* assetMaterial =
          auto_array_get(&asset.materials, glbAsset->materialIndex);

      Material* material =
          stable_auto_array_get(&materials, glbAsset->materialIndex);

      if (material->alphaMode == 0)
      {
        render_instance_queue_mesh_draw(
            assetMesh, material, glbAsset->transform, renderInstance);
      }
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

  LOG_DEBUG("Cleaning up resources.");

  for (uint32_t i = 0; i < buildingMesh.size; i++)
  {
    // TODO: Properly package assets
    Mesh* assetMesh = auto_array_get(&buildingMesh, i);
    mesh_destroy(assetMesh, renderInstance->logicalDevice);
  }
  auto_array_destroy(&buildingMesh);

  for (uint32_t i = 0; i < textureImages.size; i++)
  {
    Texture* texture = auto_array_get(&textureImages, i);
    texture_destroy(texture, renderInstance->logicalDevice);
  }
  auto_array_destroy(&textureImages);

  system_registry_destroy(&systemRegistry);
  entity_component_map_destroy(&entityComponentMap);
  stable_auto_array_destroy(&materials);
  texture_destroy(&defaultTexture, renderInstance->logicalDevice);
  game_config_destroy(&config);
  input_map_destroy(&inputMap);
  task_scheduler_destroy();
  mesh_destroy(&cubeMesh, renderInstance->logicalDevice);
  render_instance_destroy(renderInstance);
  game_window_destroy(window);
  profiler_destroy();

  return 0;
}

