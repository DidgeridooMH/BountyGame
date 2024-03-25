#include "Otter/Render/Raytracing/BoundingVolumeHierarchy.h"

#include "Otter/Async/Scheduler.h"

#define SUBDIVISION_LIMIT 5

// TODO: Move to separate file.
static void aabb_adjust_bounds(AABB* bounds, Vec3* position)
{
  bounds->x.val[0] = fminf(position->x, bounds->x.val[0]);
  bounds->y.val[0] = fminf(position->y, bounds->y.val[0]);
  bounds->z.val[0] = fminf(position->z, bounds->z.val[0]);

  bounds->x.val[1] = fmaxf(position->x, bounds->x.val[1]);
  bounds->y.val[1] = fmaxf(position->y, bounds->y.val[1]);
  bounds->z.val[1] = fmaxf(position->z, bounds->z.val[1]);
}

void bounding_volume_hierarchy_create(BoundingVolumeHierarchy* bvh)
{
  memset(bvh, 0, sizeof(BoundingVolumeHierarchy));

  stable_auto_array_create(&bvh->nodes, sizeof(BoundingVolumeNode), 32);
  BoundingVolumeNode* root = stable_auto_array_allocate(&bvh->nodes);
  memset(root, 0, sizeof(BoundingVolumeNode));

  // TODO: Adjust API to allow for larger resizes.
  auto_array_create(&bvh->primitives, sizeof(size_t));

  InitializeCriticalSection(&bvh->nodesLock);
}

void bounding_volume_hierarchy_add_primitives(MeshVertex* vertices,
    size_t numOfVertices, uint16_t* indices, size_t numOfIndices,
    Transform* transform, BoundingVolumeHierarchy* bvh)
{
  Mat4 transformMat;
  mat4_identity(transformMat);
  transform_apply(transformMat, transform);

  // TODO: Fix allocation checks.
  // TODO: Rework auto array to allow for chunk allocation.
  size_t newVertexSize = sizeof(Vec4) * (numOfVertices + bvh->numOfVertices);
  if (newVertexSize > bvh->capacityOfVertices)
  {
    bvh->vertices           = realloc(bvh->vertices, newVertexSize);
    bvh->capacityOfVertices = newVertexSize;
  }
  size_t newIndicesSize = sizeof(size_t) * (numOfIndices + bvh->numOfIndices);
  if (newIndicesSize > bvh->capacityOfIndices)
  {
    bvh->indices           = realloc(bvh->indices, newIndicesSize);
    bvh->capacityOfIndices = newIndicesSize;
  }

  BoundingVolumeNode* rootNode = stable_auto_array_get(&bvh->nodes, 0);
  Vec4* vertexRoot             = &bvh->vertices[bvh->numOfVertices];
  for (size_t i = 0; i < numOfVertices; i++)
  {
    vertexRoot[i].xyz = vertices[i].position;
    vec4_multiply_mat4(&vertexRoot[i], transformMat);
    aabb_adjust_bounds(&rootNode->bounds, &vertexRoot[i].xyz);
  }

  size_t* indicesRoot = &bvh->indices[bvh->numOfIndices];
  for (size_t i = 0; i < numOfIndices; i++)
  {
    if (i % 3 == 0)
    {
      Vec3 center = vertexRoot[indices[i]].xyz;
      vec3_add(&center, &vertexRoot[indices[i + 1]].xyz);
      vec3_add(&center, &vertexRoot[indices[i + 2]].xyz);
      aabb_adjust_bounds(&rootNode->centerBounds, &center);
    }
    indicesRoot[i] = (size_t) indices[i] + bvh->numOfVertices;
  }

  bvh->numOfVertices += numOfVertices;
  bvh->numOfIndices += numOfIndices;

  for (size_t i = 0; i < numOfIndices / 3; i++)
  {
    size_t* prim = auto_array_allocate(&bvh->primitives);
    *prim        = i;
  }
}

typedef struct BVHSubdivideData
{
  BoundingVolumeNode* node;
  BoundingVolumeHierarchy* bvh;
  int subdivideLimit;
} BVHSubdivideData;

static void bounding_volume_hierarchy_subdivide(BVHSubdivideData* bvhData)
{
  if (bvhData->node->numOfPrimitives <= 1 || bvhData->subdivideLimit == 0)
  {
#ifdef DEBUG_BVH
    for (int i = 0; i < SUBDIVISION_LIMIT - subdivideLimit; i++)
    {
      printf("\t");
    }
    printf("Split has %lld prims\n", node->numOfPrimitives);
#endif
    return;
  }

  Vec3 axisRange   = {bvhData->node->centerBounds.x.val[1]
                          - bvhData->node->centerBounds.x.val[0],
        bvhData->node->centerBounds.y.val[1]
            - bvhData->node->centerBounds.y.val[0],
        bvhData->node->centerBounds.z.val[1]
            - bvhData->node->centerBounds.z.val[0]};
  size_t splitAxis = 0;
  if (axisRange.val[splitAxis] < axisRange.y)
  {
    splitAxis = 1;
  }
  if (axisRange.val[splitAxis] < axisRange.z)
  {
    splitAxis = 2;
  }

  float splitPoint = (bvhData->node->centerBounds.axis[splitAxis].val[1]
                         + bvhData->node->centerBounds.axis[splitAxis].val[0])
                   / 2.0f;

  EnterCriticalSection(&bvhData->bvh->nodesLock);
  bvhData->node->left  = stable_auto_array_allocate(&bvhData->bvh->nodes);
  bvhData->node->right = stable_auto_array_allocate(&bvhData->bvh->nodes);
  LeaveCriticalSection(&bvhData->bvh->nodesLock);

  memset(bvhData->node->left, 0, sizeof(BoundingVolumeNode));
  bvhData->node->left->primitives      = bvhData->node->primitives;
  bvhData->node->left->numOfPrimitives = bvhData->node->numOfPrimitives;

  memset(bvhData->node->right, 0, sizeof(BoundingVolumeNode));
  bvhData->node->right->primitives =
      &bvhData->node->primitives[bvhData->node->numOfPrimitives];

  size_t* cursor = bvhData->node->primitives;
  while (cursor < bvhData->node->right->primitives)
  {
    size_t prim = *cursor;
    Vec3 primCenter =
        bvhData->bvh->vertices[bvhData->bvh->indices[prim * 3]].xyz;
    vec3_add(&primCenter,
        &bvhData->bvh->vertices[bvhData->bvh->indices[prim * 3 + 1]].xyz);
    vec3_add(&primCenter,
        &bvhData->bvh->vertices[bvhData->bvh->indices[prim * 3 + 2]].xyz);

    float center = primCenter.val[splitAxis] / 3.0f;
    vec3_divide(&primCenter, 3.0f);

    if (center > splitPoint)
    {
      bvhData->node->right->primitives -= 1;
      *cursor                           = *bvhData->node->right->primitives;
      *bvhData->node->right->primitives = prim;
      bvhData->node->right->numOfPrimitives += 1;
      bvhData->node->left->numOfPrimitives -= 1;

      aabb_adjust_bounds(&bvhData->node->right->bounds,
          &bvhData->bvh->vertices[bvhData->bvh->indices[prim * 3]].xyz);
      aabb_adjust_bounds(&bvhData->node->right->bounds,
          &bvhData->bvh->vertices[bvhData->bvh->indices[prim * 3 + 1]].xyz);
      aabb_adjust_bounds(&bvhData->node->right->bounds,
          &bvhData->bvh->vertices[bvhData->bvh->indices[prim * 3 + 2]].xyz);
      aabb_adjust_bounds(&bvhData->node->right->centerBounds, &primCenter);
    }
    else
    {
      aabb_adjust_bounds(&bvhData->node->left->bounds,
          &bvhData->bvh->vertices[bvhData->bvh->indices[prim * 3]].xyz);
      aabb_adjust_bounds(&bvhData->node->left->bounds,
          &bvhData->bvh->vertices[bvhData->bvh->indices[prim * 3 + 1]].xyz);
      aabb_adjust_bounds(&bvhData->node->left->bounds,
          &bvhData->bvh->vertices[bvhData->bvh->indices[prim * 3 + 2]].xyz);
      aabb_adjust_bounds(&bvhData->node->left->centerBounds, &primCenter);
      cursor += 1;
    }
  }

#ifdef DEBUG_BVH
  for (int i = 0; i < SUBDIVISION_LIMIT - subdivideLimit; i++)
  {
    printf("\t");
  }
  printf("Split has %lld prims\n", node->numOfPrimitives);
#endif

  HANDLE tasks[2] = {NULL};
  uint32_t count  = 0;
  if (bvhData->node->left->numOfPrimitives > 0)
  {
    BVHSubdivideData* data = malloc(sizeof(BVHSubdivideData));
    data->bvh              = bvhData->bvh;
    data->node             = bvhData->node->left;
    data->subdivideLimit   = bvhData->subdivideLimit - 1;
    tasks[count++] = task_scheduler_enqueue(bounding_volume_hierarchy_subdivide,
        data, TASK_FLAGS_FREE_DATA_ON_COMPLETE);
  }
  if (bvhData->node->right->numOfPrimitives > 0)
  {
    BVHSubdivideData* data = malloc(sizeof(BVHSubdivideData));
    data->bvh              = bvhData->bvh;
    data->node             = bvhData->node->right;
    data->subdivideLimit   = bvhData->subdivideLimit - 1;
    tasks[count++] = task_scheduler_enqueue(bounding_volume_hierarchy_subdivide,
        data, TASK_FLAGS_FREE_DATA_ON_COMPLETE);
  }
  WaitForMultipleObjects(count, tasks, true, INFINITE);
}

void bounding_volume_hierarchy_build(BoundingVolumeHierarchy* bvh)
{
  BoundingVolumeNode* root = stable_auto_array_get(&bvh->nodes, 0);
  root->primitives         = bvh->primitives.buffer;
  root->numOfPrimitives    = bvh->primitives.size;

  BVHSubdivideData* data = malloc(sizeof(BVHSubdivideData));
  data->bvh              = bvh;
  data->node             = root;
  data->subdivideLimit   = SUBDIVISION_LIMIT;
  HANDLE task = task_scheduler_enqueue(bounding_volume_hierarchy_subdivide,
      data, TASK_FLAGS_FREE_DATA_ON_COMPLETE);
  WaitForSingleObject(task, INFINITE);
}

void bounding_volume_hierarchy_reset(BoundingVolumeHierarchy* bvh)
{
  stable_auto_array_clear(&bvh->nodes);
  BoundingVolumeNode* root = stable_auto_array_allocate(&bvh->nodes);
  memset(root, 0, sizeof(BoundingVolumeNode));

  auto_array_clear(&bvh->primitives);

  bvh->numOfVertices = 0;
  bvh->numOfIndices  = 0;
}

void bounding_volume_hierarchy_destroy(BoundingVolumeHierarchy* bvh)
{
  stable_auto_array_destroy(&bvh->nodes);
  auto_array_destroy(&bvh->primitives);
  free(bvh->vertices);
  free(bvh->indices);

  DeleteCriticalSection(&bvh->nodesLock);
}
