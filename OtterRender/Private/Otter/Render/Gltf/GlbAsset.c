#include "Otter/Render/Gltf/GlbAsset.h"

#include "Otter/Util/Array/AutoArray.h"

#define STB_IMAGE_IMPLEMENTATION
#include "Extern/stb_image.h"
#include "Otter/Async/Scheduler.h"
#include "Otter/Render/Gltf/GlbJsonChunk.h"
#include "Otter/Util/Json/Json.h"
#include "Otter/Util/Log.h"

#define GLB_MAGIC 0x46546C67

typedef struct GlbHeader
{
  uint32_t magic;
  uint32_t version;
  uint32_t length;
} GlbHeader;

enum GlbChunkType
{
  GCT_JSON = 0x4E4F534A,
  GCT_BIN  = 0x004E4942
};

typedef struct GlbChunk
{
  uint32_t length;
  enum GlbChunkType type;
  char data[];
} GlbChunk;

typedef struct MeshLoadParams
{
  GlbMeshPrimitive* primitive;
  AutoArray* accessors;
  AutoArray* bufferViews;
  char* binaryData;
  size_t assetMeshIndex;
  AutoArray* assetMeshes;
  GlbNode* node;
} MeshLoadParams;

typedef struct TextureLoadParams
{
  GlbBufferView* bufferView;
  size_t index;
  GlbChunk* binaryChunk;
  GlbAssetImage* assetImage;
} TextureLoadParams;

static void glb_json_chunk_load_texture(TextureLoadParams* params, int threadId)
{
  (void) threadId;

  params->assetImage->data = stbi_load_from_memory(
      (uint8_t*) &params->binaryChunk->data[params->bufferView->offset],
      params->bufferView->length, &params->assetImage->width,
      &params->assetImage->height, &params->assetImage->channels,
      STBI_rgb_alpha);
  params->assetImage->channels = 4;

  if (params->assetImage->data == NULL)
  {
    LOG_ERROR("Unable to load image.");
  }
}

static void glb_json_chunk_load_mesh(MeshLoadParams* params, int threadId)
{
  (void) threadId;

  if (params->primitive->position < 0 || params->primitive->indices < 0)
  {
    LOG_WARNING(
        "Mesh primitive must at least have a position and index buffer.");

    GlbAssetMesh* assetMesh =
        auto_array_get(params->assetMeshes, params->assetMeshIndex);
    assetMesh->numOfVertices = 0;
    assetMesh->numOfIndices  = 0;
    return;
  }

  GlbAccessor* positionAccessor =
      auto_array_get(params->accessors, params->primitive->position);
  GlbBufferView* positionBuffer =
      auto_array_get(params->bufferViews, positionAccessor->bufferView);
  Vec3* positions = (Vec3*) &params->binaryData[positionBuffer->offset];

  GlbAccessor* indexAccessor =
      auto_array_get(params->accessors, params->primitive->indices);
  GlbBufferView* indicesBuffer =
      auto_array_get(params->bufferViews, indexAccessor->bufferView);
  uint16_t* indices = (uint16_t*) &params->binaryData[indicesBuffer->offset];

  GlbBufferView* normalBuffer = NULL;
  Vec3* normals               = NULL;
  if (params->primitive->normal >= 0)
  {
    GlbAccessor* normalAccessor =
        auto_array_get(params->accessors, params->primitive->normal);
    normalBuffer =
        auto_array_get(params->bufferViews, normalAccessor->bufferView);
    normals = (Vec3*) &params->binaryData[normalBuffer->offset];
  }

  GlbBufferView* tangentBuffer = NULL;
  Vec4* tangents               = NULL;
  if (params->primitive->tangent >= 0)
  {
    GlbAccessor* tangentAccessor =
        auto_array_get(params->accessors, params->primitive->tangent);
    tangentBuffer =
        auto_array_get(params->bufferViews, tangentAccessor->bufferView);
    tangents = (Vec4*) &params->binaryData[tangentBuffer->offset];
  }

  GlbBufferView* uvBuffer = NULL;
  Vec2* uvs               = NULL;
  if (params->primitive->uv >= 0)
  {
    GlbAccessor* uvAccessor =
        auto_array_get(params->accessors, params->primitive->uv);
    uvBuffer = auto_array_get(params->bufferViews, uvAccessor->bufferView);
    uvs      = (Vec2*) &params->binaryData[uvBuffer->offset];
  }

  GlbAssetMesh* assetMesh =
      auto_array_get(params->assetMeshes, params->assetMeshIndex);
  assetMesh->vertices = calloc(positionAccessor->count, sizeof(MeshVertex));
  assetMesh->numOfVertices = positionAccessor->count;

  assetMesh->indices      = calloc(indexAccessor->count, sizeof(uint16_t));
  assetMesh->numOfIndices = indexAccessor->count;

  for (uint32_t attribute = 0; attribute < positionAccessor->count; attribute++)
  {
    assetMesh->vertices[attribute].position = positions[attribute];
    assetMesh->vertices[attribute].position.y *= -1.0f;
    if (normals != NULL)
    {
      assetMesh->vertices[attribute].normal = normals[attribute];
      assetMesh->vertices[attribute].normal.y *= -1.0f;
    }
    if (tangents != NULL)
    {
      assetMesh->vertices[attribute].tangent = tangents[attribute];
      assetMesh->vertices[attribute].tangent.y *= -1.0f;
    }
    if (uvs != NULL)
    {
      assetMesh->vertices[attribute].uv = uvs[attribute];
    }
  }

  for (uint32_t index = 0; index < indexAccessor->count; index++)
  {
    assetMesh->indices[index] = indices[index];
  }

  memcpy(&assetMesh->transform, &params->node->transform, sizeof(Mat4));

  assetMesh->materialIndex = params->primitive->material;
}

// TODO: More closely examine where you're reading from content and ensure it
// doesn't go over the contentSize.
OTTERRENDER_API bool glb_load_asset(
    char* content, size_t contentSize, GlbAsset* asset)
{
  if (contentSize < sizeof(GlbHeader))
  {
    LOG_ERROR("Invalid GLB header.");
    return false;
  }

  GlbHeader* glbHeader = (GlbHeader*) content;
  if (glbHeader->magic != GLB_MAGIC || glbHeader->version != 2
      || glbHeader->length != contentSize)
  {
    LOG_ERROR("Invalid GLB file format.");
    return false;
  }

  GlbChunk* jsonChunk = (GlbChunk*) (content + sizeof(GlbHeader));
  if (jsonChunk->type != GCT_JSON)
  {
    LOG_ERROR("First chunk must be a JSON chunk.");
    return false;
  }

  size_t bytesParsed = 0;
  JsonValue* glbJsonData =
      json_parse(jsonChunk->data, jsonChunk->length, &bytesParsed);
  if (glbJsonData == NULL)
  {
    LOG_ERROR("Unable to parse JSON chunk");
    return false;
  }

  GlbJsonChunk parsedJsonChunk = {0};
  if (!glb_json_chunk_parse(glbJsonData, &parsedJsonChunk))
  {
    json_destroy(glbJsonData);
    return false;
  }

  GlbChunk* binaryChunk = (GlbChunk*) (jsonChunk->data + jsonChunk->length);
  if (binaryChunk->type != GCT_BIN)
  {
    LOG_ERROR("Second chunk must be a binary chunk.");
    json_destroy(glbJsonData);
    return false;
  }

  AutoArray meshLoadParams;
  auto_array_create(&meshLoadParams, sizeof(MeshLoadParams));
  auto_array_create(&asset->meshes, sizeof(GlbAssetMesh));
  for (uint32_t i = 0; i < parsedJsonChunk.nodes.size; i++)
  {
    GlbNode* node = auto_array_get(&parsedJsonChunk.nodes, i);
    if (node->type == NT_MESH)
    {
      GlbMesh* mesh = auto_array_get(&parsedJsonChunk.meshes, node->mesh);
      for (uint32_t p = 0; p < mesh->primitives.size; p++)
      {
        GlbAssetMesh* assetMesh     = auto_array_allocate(&asset->meshes);
        GlbMeshPrimitive* primitive = auto_array_get(&mesh->primitives, p);

        MeshLoadParams* taskParams = auto_array_allocate(&meshLoadParams);
        taskParams->primitive      = primitive;
        taskParams->accessors      = &parsedJsonChunk.accessors;
        taskParams->bufferViews    = &parsedJsonChunk.bufferViews;
        taskParams->binaryData     = binaryChunk->data;
        taskParams->assetMeshIndex = asset->meshes.size - 1;
        taskParams->assetMeshes    = &asset->meshes;
        taskParams->node           = node;
      }
    }
  }

  AutoArray meshLoadTasks;
  auto_array_create(&meshLoadTasks, sizeof(HANDLE));
  auto_array_allocate_many(&meshLoadTasks, meshLoadParams.size);
  for (uint32_t i = 0; i < meshLoadTasks.size; i++)
  {
    MeshLoadParams* taskParams = auto_array_get(&meshLoadParams, i);

    HANDLE* task = auto_array_get(&meshLoadTasks, i);
    *task        = task_scheduler_enqueue(
        (TaskFunction) glb_json_chunk_load_mesh, taskParams, 0);
  }

  auto_array_create(&asset->materials, sizeof(GlbAssetMaterial));
  auto_array_allocate_many(&asset->materials, parsedJsonChunk.materials.size);
  for (uint32_t i = 0; i < parsedJsonChunk.materials.size; i++)
  {
    GlbMaterial* material = auto_array_get(&parsedJsonChunk.materials, i);
    GlbAssetMaterial* assetMaterial = auto_array_get(&asset->materials, i);

    assetMaterial->baseColorTexture = material->baseColorTexture;
    assetMaterial->normalTexture    = material->normalTexture;
    assetMaterial->metallicRoughnessTexture =
        material->metallicRoughnessTexture;
    assetMaterial->occlusionTexture = material->occlusionTexture;
    assetMaterial->emissiveTexture  = material->emissiveTexture;
    memcpy(&assetMaterial->baseColor, &material->baseColorFactor, sizeof(Vec4));
    assetMaterial->metallicFactor    = material->metallicFactor;
    assetMaterial->roughnessFactor   = material->roughnessFactor;
    assetMaterial->occlusionStrength = material->occlusionStrength;
    memcpy(&assetMaterial->emissive, &material->emissiveFactor, sizeof(Vec3));
    assetMaterial->alphaMode   = material->alphaMode;
    assetMaterial->alphaCutoff = material->alphaCutoff;
  }

  auto_array_create(&asset->textures, sizeof(uint32_t));
  auto_array_allocate_many(&asset->textures, parsedJsonChunk.textures.size);
  for (uint32_t i = 0; i < parsedJsonChunk.textures.size; i++)
  {
    GlbTexture* texture    = auto_array_get(&parsedJsonChunk.textures, i);
    uint32_t* assetTexture = auto_array_get(&asset->textures, i);
    *assetTexture          = texture->source;
  }

  auto_array_create(&asset->images, sizeof(GlbAssetImage));
  auto_array_allocate_many(&asset->images, parsedJsonChunk.images.size);

  AutoArray textureLoadTasks;
  auto_array_create(&textureLoadTasks, sizeof(HANDLE));
  auto_array_allocate_many(&textureLoadTasks, asset->images.size);

  AutoArray textureLoadParams;
  auto_array_create(&textureLoadParams, sizeof(TextureLoadParams));
  auto_array_allocate_many(&textureLoadParams, asset->images.size);

  for (uint32_t i = 0; i < parsedJsonChunk.images.size; i++)
  {
    GlbImage* image = auto_array_get(&parsedJsonChunk.images, i);
    GlbBufferView* imageBuffer =
        auto_array_get(&parsedJsonChunk.bufferViews, image->bufferView);
    GlbAssetImage* assetImage = auto_array_get(&asset->images, i);
    assetImage->colorType     = image->colorType;

    TextureLoadParams* taskParams = auto_array_get(&textureLoadParams, i);
    taskParams->bufferView        = imageBuffer;
    taskParams->index             = i;
    taskParams->binaryChunk       = binaryChunk;
    taskParams->assetImage        = assetImage;

    HANDLE* task = auto_array_get(&textureLoadTasks, i);
    *task        = task_scheduler_enqueue(
        (TaskFunction) glb_json_chunk_load_texture, taskParams, 0);
  }

  LOG_DEBUG("Waiting for meshes to load");
  for (uint32_t i = 0; i < meshLoadTasks.size; i++)
  {
    HANDLE* task = auto_array_get(&meshLoadTasks, i);
    WaitForSingleObject(*task, INFINITE);
  }

  LOG_DEBUG("Waiting for textures to load");
  for (uint32_t i = 0; i < textureLoadTasks.size; i++)
  {
    HANDLE* task = auto_array_get(&textureLoadTasks, i);
    WaitForSingleObject(*task, INFINITE);
  }

  LOG_DEBUG("Loaded %d meshes, %d materials, %d textures, and %d images.",
      asset->meshes.size, asset->materials.size, asset->textures.size,
      asset->images.size);

  auto_array_destroy(&meshLoadTasks);
  auto_array_destroy(&meshLoadParams);

  auto_array_destroy(&textureLoadTasks);
  auto_array_destroy(&textureLoadParams);

  glb_json_chunk_destroy(&parsedJsonChunk);
  json_destroy(glbJsonData);

  return true;
}

