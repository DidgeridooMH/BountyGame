#pragma once

#include "Otter/Math/Transform.h"
#include "Otter/Math/Vec.h"
#include "Otter/Render/Mesh.h"
#include "Otter/Util/AutoArray.h"
#include "Otter/Util/StableAutoArray.h"

typedef struct AABB
{
  union
  {
    struct
    {
      Vec2 x;
      Vec2 y;
      Vec2 z;
    };
    Vec2 axis[3];
  };
} AABB;

typedef struct BoundingVolumeNode
{
  AABB bounds;
  AABB centerBounds;

  size_t* primitives;
  size_t numOfPrimitives;

  struct BoundingVolumeNode* left;
  struct BoundingVolumeNode* right;
} BoundingVolumeNode;

typedef struct BoundingVolumeHierarchy
{
  CRITICAL_SECTION nodesLock;
  StableAutoArray nodes;
  AutoArray primitives;

  Vec4* vertices;
  size_t numOfVertices;
  size_t capacityOfVertices;
  size_t* indices;
  size_t numOfIndices;
  size_t capacityOfIndices;
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
