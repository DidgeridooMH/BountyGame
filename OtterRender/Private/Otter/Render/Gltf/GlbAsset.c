#include "Otter/Render/Gltf/GlbAsset.h"

#include "Otter/Render/Gltf/GlbJsonChunk.h"
#include "Otter/Util/Json/Json.h"

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

// TODO: More closely examine where you're reading from content and ensure it
// doesn't go over the contentSize.
OTTERRENDER_API bool glb_load_asset(
    char* content, size_t contentSize, GlbAsset* asset)
{
  if (contentSize < sizeof(GlbHeader))
  {
    fprintf(stderr, "Invalid GLB header.\n");
    return false;
  }

  GlbHeader* glbHeader = (GlbHeader*) content;
  if (glbHeader->magic != GLB_MAGIC || glbHeader->version != 2
      || glbHeader->length != contentSize)
  {
    fprintf(stderr, "Invalid GLB file format.\n");
    return false;
  }

  GlbChunk* jsonChunk = (GlbChunk*) (content + sizeof(GlbHeader));
  if (jsonChunk->type != GCT_JSON)
  {
    fprintf(stderr, "First chunk must be a JSON chunk.\n");
    return false;
  }

  size_t bytesParsed = 0;
  JsonValue* glbJsonData =
      json_parse(jsonChunk->data, jsonChunk->length, &bytesParsed);
  if (glbJsonData == NULL)
  {
    fprintf(stderr, "Unable to parse JSON chunk\n");
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
    fprintf(stderr, "Second chunk must be a binary chunk.\n");
    json_destroy(glbJsonData);
    return false;
  }

  auto_array_create(&asset->meshes, sizeof(GlbAssetMesh));
  for (uint32_t i = 0; i < parsedJsonChunk.nodes.size; i++)
  {
    GlbNode* node = auto_array_get(&parsedJsonChunk.nodes, i);
    GlbMesh* mesh = auto_array_get(&parsedJsonChunk.meshes, node->mesh);
    for (uint32_t p = 0; p < mesh->primitives.size; p++)
    {
      // TODO: Do some checks to verify integrity.
      GlbMeshPrimitive* primitive = auto_array_get(&mesh->primitives, p);
      GlbAccessor* positionAccessor =
          auto_array_get(&parsedJsonChunk.accessors, primitive->position);
      GlbAccessor* normalAccessor =
          auto_array_get(&parsedJsonChunk.accessors, primitive->normal);
      GlbAccessor* uvAccessor =
          auto_array_get(&parsedJsonChunk.accessors, primitive->uv);
      GlbAccessor* indexAccessor =
          auto_array_get(&parsedJsonChunk.accessors, primitive->indices);

      GlbBufferView* positionBuffer = auto_array_get(
          &parsedJsonChunk.bufferViews, positionAccessor->bufferView);
      GlbBufferView* normalBuffer = auto_array_get(
          &parsedJsonChunk.bufferViews, normalAccessor->bufferView);
      GlbBufferView* uvBuffer =
          auto_array_get(&parsedJsonChunk.bufferViews, uvAccessor->bufferView);
      GlbBufferView* indicesBuffer = auto_array_get(
          &parsedJsonChunk.bufferViews, indexAccessor->bufferView);

      // TODO: Figure out how multiple buffers would work.
      Vec3* positions   = (Vec3*) &binaryChunk->data[positionBuffer->offset];
      Vec3* normals     = (Vec3*) &binaryChunk->data[normalBuffer->offset];
      Vec2* uvs         = (Vec2*) &binaryChunk->data[uvBuffer->offset];
      uint16_t* indices = (uint16_t*) &binaryChunk->data[indicesBuffer->offset];

      GlbAssetMesh* assetMesh = auto_array_allocate(&asset->meshes);

      assetMesh->vertices =
          malloc(sizeof(MeshVertex) * positionAccessor->count);
      assetMesh->numOfVertices = positionAccessor->count;

      assetMesh->indices      = malloc(sizeof(uint16_t) * indexAccessor->count);
      assetMesh->numOfIndices = indexAccessor->count;

      for (uint32_t attribute = 0; attribute < positionAccessor->count;
           attribute++)
      {
        // TODO: Take into consideration the buffer index.
        assetMesh->vertices[attribute].position = positions[attribute];
        assetMesh->vertices[attribute].normal   = normals[attribute];
        assetMesh->vertices[attribute].uv       = uvs[attribute];
      }

      for (uint32_t index = 0; index < indexAccessor->count; index++)
      {
        assetMesh->indices[index] = indices[index];
      }

      assetMesh->transform = node->transform;
      assetMesh->transform.position.y *= -1;
      assetMesh->transform.scale.y *= -1;
    }
  }

  glb_json_chunk_destroy(&parsedJsonChunk);
  json_destroy(glbJsonData);

  return true;
}
