#pragma once

#include "Otter/Math/MatDef.h"
#include "Otter/Math/Vec.h"
#include "Otter/Util/AutoArray.h"
#include "Otter/Util/Json/Json.h"

typedef enum NodeType
{
  NT_EMPTY,
  NT_MESH
} NodeType;

typedef struct GlbNode
{
  NodeType type;
  AutoArray children;
  uint32_t mesh;
  Mat4 transform;
} GlbNode;

typedef struct GlbMeshPrimitive
{
  uint32_t position;
  uint32_t normal;
  uint32_t uv;
  uint32_t indices;
} GlbMeshPrimitive;

typedef struct GlbMesh
{
  AutoArray primitives;
} GlbMesh;

enum GlbComponentType
{
  GCT_BYTE           = 5120,
  GCT_UNSIGNED_BYTE  = 5121,
  GCT_SHORT          = 5122,
  GCT_UNSIGNED_SHORT = 5123,
  GCT_UNSIGNED_INT   = 5125,
  GCT_FLOAT          = 5126
};

enum GlbRank
{
  GR_SCALAR,
  GR_VEC2,
  GR_VEC3,
  GR_VEC4,
  GR_MAT2,
  GR_MAT3,
  GR_MAT4
};

typedef struct GlbAccessor
{
  uint32_t bufferView;
  enum GlbComponentType componentType;
  enum GlbRank rank;
  uint32_t count;
  bool useBounds;
  Vec3 min;
  Vec3 max;
} GlbAccessor;

typedef struct GlbBufferView
{
  uint32_t buffer;
  uint32_t length;
  uint32_t offset;
} GlbBufferView;

typedef struct GlbBuffer
{
  uint32_t byteLength;
} GlbBuffer;

typedef struct GlbJsonChunk
{
  AutoArray nodes;
  AutoArray meshes;
  AutoArray accessors;
  AutoArray bufferViews;
  AutoArray buffers;
} GlbJsonChunk;

bool glb_json_chunk_parse(JsonValue* json, GlbJsonChunk* jsonChunk);

void glb_json_chunk_destroy(GlbJsonChunk* jsonChunk);

