#pragma once

#include "Otter/Render/Mesh.h"
#include "Otter/Render/export.h"
#include "Otter/Util/AutoArray.h"

typedef struct GlbAssetMesh
{
  MeshVertex* vertices;
  uint32_t numOfVertices;
  uint16_t* indices;
  uint32_t numOfIndices;
  Mat4 transform;
  uint32_t materialIndex;
} GlbAssetMesh;

typedef struct GlbAssetTexture
{
  uint8_t* data;
  uint32_t width;
  uint32_t height;
  uint32_t channels;
} GlbAssetTexture;

typedef struct GlbAssetMaterial
{
  uint32_t baseColorTexture;
  uint32_t normalTexture;
  uint32_t metallicRoughnessTexture;
  uint32_t emissiveTexture;
  Vec4 baseColor;
  float metallicFactor;
  float roughnessFactor;
  Vec3 emissive;
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
