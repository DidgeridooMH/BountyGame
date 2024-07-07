#include "Otter/Render/Raytracing/BoundingVolumeHierarchy.h"

#include "Otter/Async/Scheduler.h"
#include "Otter/Util/Log.h"

#define SUBDIVISION_LIMIT 20
// #define DEBUG_BVH

typedef struct BVHSubdivideData
{
  BoundingVolumeNode* node;
  BoundingVolumeHierarchy* bvh;
  int subdivideLimit;
} BVHSubdivideData;

void bounding_volume_hierarchy_create(BoundingVolumeHierarchy* bvh)
{
  memset(bvh, 0, sizeof(BoundingVolumeHierarchy));

  stable_auto_array_create(&bvh->nodes, sizeof(BoundingVolumeNode), 128);

  auto_array_create(&bvh->vertices, sizeof(Vec4));
  auto_array_create(&bvh->tris, sizeof(Triangle));

  InitializeCriticalSection(&bvh->nodesLock);

  bounding_volume_hierarchy_reset(bvh);
}

static void bounding_volume_hierarchy_get_centroid(
    Vec3* result, BoundingVolumeHierarchy* bvh, Triangle* tri)
{
  Vec4* vertices = auto_array_get(&bvh->vertices, 0);

  result->x = 0;
  result->y = 0;
  result->z = 0;

  vec3_add(result, &vertices[tri->a].xyz);
  vec3_add(result, &vertices[tri->b].xyz);
  vec3_add(result, &vertices[tri->c].xyz);
  vec3_divide(result, 3);
}

void bounding_volume_hierarchy_add_primitives(MeshVertex* vertices,
    size_t numOfVertices, uint16_t* indices, size_t numOfIndices,
    Transform* transform, BoundingVolumeHierarchy* bvh)
{
  Mat4 transformMat;
  mat4_identity(transformMat);
  transform_apply(transformMat, transform);

  BoundingVolumeNode* rootNode = stable_auto_array_get(&bvh->nodes, 0);

  size_t vertexLocalOffset = bvh->vertices.size;
  Vec4* vertexRoot = auto_array_allocate_many(&bvh->vertices, numOfVertices);
  for (size_t i = 0; i < numOfVertices; i++)
  {
    vertexRoot[i].xyz = vertices[i].position;
    vertexRoot[i].w   = 1.0;
    // TODO: SSE is your friend.
    vec4_multiply_mat4(&vertexRoot[i], transformMat);
    aabb_adjust_bounds(&rootNode->bounds, &vertexRoot[i].xyz);
  }

  Triangle* trisRoot = auto_array_allocate_many(&bvh->tris, numOfIndices / 3);
  for (size_t i = 0; i < numOfIndices / 3; i++)
  {
    trisRoot[i].a = indices[i * 3] + vertexLocalOffset;
    trisRoot[i].b = indices[i * 3 + 1] + vertexLocalOffset;
    trisRoot[i].c = indices[i * 3 + 2] + vertexLocalOffset;

    Vec3 centroid;
    bounding_volume_hierarchy_get_centroid(&centroid, bvh, &trisRoot[i]);

    aabb_adjust_bounds(&rootNode->centerBounds, &centroid);
    vec3_add(&rootNode->centerCluster, &centroid);
  }
}

static void bounding_volume_hierarchy_subdivide(BVHSubdivideData* bvhData)
{
  if (bvhData->node->numOfTris <= 10 || bvhData->subdivideLimit == 0)
  {
    return;
  }

  vec3_divide(&bvhData->node->centerCluster, (float) bvhData->node->numOfTris);

  Vec3 axisRange = bvhData->node->centerBounds.max;
  vec3_subtract(&axisRange, &bvhData->node->centerBounds.min);

  size_t splitAxis = vec3_max_index(&axisRange);

  float splitPoint = bvhData->node->centerCluster.val[splitAxis];

  EnterCriticalSection(&bvhData->bvh->nodesLock);
  bvhData->node->left  = stable_auto_array_allocate(&bvhData->bvh->nodes);
  bvhData->node->right = stable_auto_array_allocate(&bvhData->bvh->nodes);
  LeaveCriticalSection(&bvhData->bvh->nodesLock);

  memset(bvhData->node->left, 0, sizeof(BoundingVolumeNode));
  bvhData->node->left->tris      = bvhData->node->tris;
  bvhData->node->left->numOfTris = bvhData->node->numOfTris;
  aabb_initialize(&bvhData->node->left->bounds);
  aabb_initialize(&bvhData->node->left->centerBounds);

  memset(bvhData->node->right, 0, sizeof(BoundingVolumeNode));
  bvhData->node->right->tris = bvhData->node->tris + bvhData->node->numOfTris;
  aabb_initialize(&bvhData->node->right->bounds);
  aabb_initialize(&bvhData->node->right->centerBounds);

  Triangle* cursor = bvhData->node->left->tris;
  while (cursor < bvhData->node->right->tris)
  {
    Vec3 primCenter;
    bounding_volume_hierarchy_get_centroid(&primCenter, bvhData->bvh, cursor);

    Triangle currentTriangle       = *cursor;
    BoundingVolumeNode* chosenNode = bvhData->node->left;
    if (primCenter.val[splitAxis] > splitPoint)
    {
      chosenNode = bvhData->node->right;
      bvhData->node->right->tris -= 1;
      *cursor                     = *bvhData->node->right->tris;
      *bvhData->node->right->tris = currentTriangle;

      bvhData->node->right->numOfTris += 1;
      bvhData->node->left->numOfTris -= 1;
    }
    else
    {
      cursor += 1;
    }

    Vec4* v0 = auto_array_get(&bvhData->bvh->vertices, currentTriangle.a);
    Vec4* v1 = auto_array_get(&bvhData->bvh->vertices, currentTriangle.b);
    Vec4* v2 = auto_array_get(&bvhData->bvh->vertices, currentTriangle.c);

    aabb_adjust_bounds(&chosenNode->bounds, &v0->xyz);
    aabb_adjust_bounds(&chosenNode->bounds, &v1->xyz);
    aabb_adjust_bounds(&chosenNode->bounds, &v2->xyz);
    aabb_adjust_bounds(&chosenNode->centerBounds, &primCenter);
    vec3_add(&chosenNode->centerCluster, &primCenter);
  }

  BVHSubdivideData leftData = {.bvh = bvhData->bvh,
      .node                         = bvhData->node->left,
      .subdivideLimit               = bvhData->subdivideLimit - 1};
  if (bvhData->node->left->numOfTris > 0)
  {
    bounding_volume_hierarchy_subdivide(&leftData);
  }

  BVHSubdivideData rightData = {.bvh = bvhData->bvh,
      .node                          = bvhData->node->right,
      .subdivideLimit                = bvhData->subdivideLimit - 1};
  if (bvhData->node->right->numOfTris > 0)
  {
    bounding_volume_hierarchy_subdivide(&rightData);
  }
}

#ifdef DEBUG_BVH
static void bounding_volume_hierarchy_print(BoundingVolumeNode* node, int level)
{
  for (int i = 0; i < level; i++)
  {
    // TODO: Fix this to work with logger.
    printf("  ");
  }
  LOG_DEBUG("Split has %lld prims", node->numOfTris);
  if (node->left != NULL)
  {
    bounding_volume_hierarchy_print(node->left, level + 1);
  }
  if (node->right != NULL)
  {
    bounding_volume_hierarchy_print(node->right, level + 1);
  }
}
#endif

void bounding_volume_hierarchy_build(BoundingVolumeHierarchy* bvh)
{
  BoundingVolumeNode* root = stable_auto_array_get(&bvh->nodes, 0);
  root->tris               = auto_array_get(&bvh->tris, 0);
  root->numOfTris          = bvh->tris.size;

  BVHSubdivideData data = {
      .bvh = bvh, .node = root, .subdivideLimit = SUBDIVISION_LIMIT};

  HANDLE task = task_scheduler_enqueue(
      (void (*)(void*)) bounding_volume_hierarchy_subdivide, &data, 0);

  WaitForSingleObject(task, INFINITE);
  CloseHandle(task);

#ifdef DEBUG_BVH
  bounding_volume_hierarchy_print(root, 0);
#endif
}

void bounding_volume_hierarchy_reset(BoundingVolumeHierarchy* bvh)
{
  EnterCriticalSection(&bvh->nodesLock);

  stable_auto_array_clear(&bvh->nodes);
  BoundingVolumeNode* root = stable_auto_array_allocate(&bvh->nodes);
  memset(root, 0, sizeof(BoundingVolumeNode));
  aabb_initialize(&root->bounds);
  aabb_initialize(&root->centerBounds);

  auto_array_clear(&bvh->vertices);
  auto_array_clear(&bvh->tris);

  LeaveCriticalSection(&bvh->nodesLock);
}

void bounding_volume_hierarchy_destroy(BoundingVolumeHierarchy* bvh)
{
  EnterCriticalSection(&bvh->nodesLock);
  stable_auto_array_destroy(&bvh->nodes);
  auto_array_destroy(&bvh->vertices);
  auto_array_destroy(&bvh->tris);
  LeaveCriticalSection(&bvh->nodesLock);
  DeleteCriticalSection(&bvh->nodesLock);
}
