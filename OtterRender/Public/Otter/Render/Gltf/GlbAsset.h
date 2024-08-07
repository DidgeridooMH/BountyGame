#pragma once

#include "Otter/Render/Gltf/GlbJsonChunk.h"
#include "Otter/Render/Mesh.h"
#include "Otter/Render/export.h"
#include "Otter/Util/Array/AutoArray.h"

typedef struct GlbAssetMesh
{
  MeshVertex* vertices;
  uint32_t numOfVertices;
  uint16_t* indices;
  uint32_t numOfIndices;
  Mat4 transform;
  uint32_t materialIndex;
} GlbAssetMesh;

typedef struct GlbAssetImage
{
  uint8_t* data;
  int width;
  int height;
  int channels;
  GlbImageColorType colorType;
} GlbAssetImage;

typedef struct GlbAssetMaterial
{
  uint32_t baseColorTexture;
  uint32_t normalTexture;
  uint32_t metallicRoughnessTexture;
  uint32_t occlusionTexture;
  uint32_t emissiveTexture;
  Vec4 baseColor;
  float metallicFactor;
  float roughnessFactor;
  float occlusionStrength;
  Vec3 emissive;
  GlbMaterialAlphaMode alphaMode;
  float alphaCutoff;
} GlbAssetMaterial;

typedef struct GlbAsset
{
  // @brief Meshes that are in the glb file.
  AutoArray meshes;
  // @brief Materials that are in the glb file.
  AutoArray materials;
  // @brief Index array that maps to an image.
  AutoArray textures;
  // @brief Images that are in the glb file.
  AutoArray images;
} GlbAsset;

OTTERRENDER_API bool glb_load_asset(
    char* content, size_t contentSize, GlbAsset* asset);
