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
} GlbAssetMesh;

typedef struct GlbAssetMaterial
{
  Vec4 baseColorFactor;
  size_t baseColorTexture;
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
