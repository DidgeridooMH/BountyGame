#pragma once

#include "Otter/Math/Transform.h"
#include "Otter/Render/Mesh.h"
#include "Otter/Render/export.h"
#include "Otter/Util/AutoArray.h"

typedef struct GlbMesh
{
  MeshVertex vertices;
  uint32_t numOfVertices;
} GlbMesh;

typedef struct GlbNode
{
  uint32_t mesh;
  Transform transform;
} GlbNode;

struct GlbAsset
{
  GlbMesh meshes;
  uint32_t numOfMeshes;
};

OTTERRENDER_API bool glb_load_asset(
    char* content, size_t contentSize, AutoArray* nodes);
