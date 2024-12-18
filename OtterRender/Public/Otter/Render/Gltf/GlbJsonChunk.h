#pragma once

#include "Otter/Math/MatDef.h"
#include "Otter/Math/Vec.h"
#include "Otter/Util/Array/AutoArray.h"
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
  int32_t position;
  int32_t normal;
  int32_t uv;
  int32_t tangent;
  int32_t indices;
  int32_t material;
} GlbMeshPrimitive;

typedef enum GlbMaterialAlphaMode
{
  GLB_MATERIAL_ALPHA_MODE_OPAQUE,
  GLB_MATERIAL_ALPHA_MODE_MASK,
  GLB_MATERIAL_ALPHA_MODE_BLEND
} GlbMaterialAlphaMode;

typedef struct GlbMaterial
{
  Vec4 baseColorFactor;
  uint32_t baseColorTexture;
  float metallicFactor;
  float roughnessFactor;
  uint32_t metallicRoughnessTexture;
  uint32_t normalTexture;
  uint32_t occlusionTexture;
  float occlusionStrength;
  Vec3 emissiveFactor;
  uint32_t emissiveTexture;
  GlbMaterialAlphaMode alphaMode;
  float alphaCutoff;
} GlbMaterial;

typedef struct GlbTexture
{
  uint32_t source;
  uint32_t sampler;
} GlbTexture;

typedef enum GlbImageMimeType
{
  GIM_JPEG,
  GIM_PNG
} GlbImageMimeType;

typedef enum GlbImageColorType
{
  GICT_SRGB,
  GICT_LINEAR
} GlbImageColorType;

typedef struct GlbImage
{
  uint32_t bufferView;
  GlbImageMimeType mimeType;
  GlbImageColorType colorType;
} GlbImage;

// TODO: Implement samplers.

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
  AutoArray materials;
  AutoArray textures;
  AutoArray images;
} GlbJsonChunk;

bool glb_json_chunk_parse(JsonValue* json, GlbJsonChunk* jsonChunk);

void glb_json_chunk_destroy(GlbJsonChunk* jsonChunk);
