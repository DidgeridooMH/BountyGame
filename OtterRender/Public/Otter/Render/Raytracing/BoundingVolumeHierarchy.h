#pragma once

#include "Otter/Math/Transform.h"
#include "Otter/Math/Vec.h"
#include "Otter/Render/Mesh.h"
#include "Otter/Render/Raytracing/AABB.h"
#include "Otter/Util/AutoArray.h"
#include "Otter/Util/StableAutoArray.h"

typedef struct Triangle
{
  size_t a;
  size_t b;
  size_t c;
} Triangle;

typedef struct BoundingVolumeNode
{
  AABB bounds;
  AABB centerBounds;
  Vec3 centerCluster;

  Triangle* tris;
  size_t numOfTris;

  struct BoundingVolumeNode* left;
  struct BoundingVolumeNode* right;
} BoundingVolumeNode;

typedef struct BoundingVolumeHierarchy
{
  CRITICAL_SECTION nodesLock;
  StableAutoArray nodes;

  // Vec4
  AutoArray vertices;

  // Triangle
  AutoArray tris;
} BoundingVolumeHierarchy;

OTTERRENDER_API void bounding_volume_hierarchy_create(
    BoundingVolumeHierarchy* bvh);

OTTERRENDER_API void bounding_volume_hierarchy_add_primitives(
    MeshVertex* vertices, size_t numOfVertices, uint16_t* indices,
    size_t numOfIndices, Transform* transform, BoundingVolumeHierarchy* bvh);

OTTERRENDER_API void bounding_volume_hierarchy_build(
    BoundingVolumeHierarchy* bvh);

OTTERRENDER_API void bounding_volume_hierarchy_reset(
    BoundingVolumeHierarchy* bvh);

OTTERRENDER_API void bounding_volume_hierarchy_destroy(
    BoundingVolumeHierarchy* bvh);
