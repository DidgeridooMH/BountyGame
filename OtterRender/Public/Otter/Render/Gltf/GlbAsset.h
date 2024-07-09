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

typedef struct GlbAsset
{
  AutoArray meshes;
} GlbAsset;

OTTERRENDER_API bool glb_load_asset(
    char* content, size_t contentSize, GlbAsset* asset);

