#pragma once

#include "Otter/Math/Transform.h"
#include "Otter/Render/Mesh.h"
#include "Otter/Render/export.h"
#include "Otter/Util/AutoArray.h"

typedef struct GlbAssetMesh
{
  MeshVertex* vertices;
  uint32_t numOfVertices;
  uint16_t* indices;
  uint32_t numOfIndices;
  Transform transform;
} GlbAssetMesh;

typedef struct GlbAsset
{
  AutoArray meshes;
} GlbAsset;

OTTERRENDER_API bool glb_load_asset(
    char* content, size_t contentSize, GlbAsset* asset);
