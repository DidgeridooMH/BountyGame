#include "Otter/Render/Gltf/GlbAsset.h"

#include "Otter/Util/Json/Json.h"

#define GLB_MAGIC                0x46546C67
#define GLB_ARRAY_BUFFER         0x8891
#define GLB_ELEMENT_ARRAY_BUFFER 0x8893

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
    char* content, size_t contentSize, AutoArray* nodes)
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

  auto_array_create(nodes, sizeof(GlbNode));
  /*JsonValue* glbNodes  = hash_map_get_value(glbJsonData->object, "nodes");
  JsonValue* glbMeshes = hash_map_get_value(glbJsonData->object, "meshes");
  if (glbNodes != NULL && glbNodes->type == JT_ARRAY && glbMeshes != NULL
      && glbMeshes->type == JT_ARRAY)
  {
    for (uint32_t i = 0; i < glbNodes->array.size; i++)
    {
      JsonValue* currentNode =
          *(JsonValue**) auto_array_get(&glbNodes->array, i);
      if (currentNode->type == JT_OBJECT)
      {
        JsonValue* currentMesh =
            hash_map_get_value(currentNode->object, "mesh");
        if (currentMesh != NULL && currentMesh->type == JT_NUMBER)
        {
          printf("Mesh -> %d\n", (int) currentMesh->number);
        }
      }
    }
  }*/

  json_destroy(glbJsonData);

  return true;
}
