#include "Otter/Render/Raytracing/Ray.h"

#include <immintrin.h>

void ray_create(Ray* ray, const Vec3* origin, const Vec3* direction)
{
  ray->origin    = *origin;
  ray->direction = *direction;

  ray->inverseDirection.x =
      direction->x != 0.0f ? 1.0f / direction->x : INFINITY;
  ray->inverseDirection.y =
      direction->y != 0.0f ? 1.0f / direction->y : INFINITY;
  ray->inverseDirection.z =
      direction->z != 0.0f ? 1.0f / direction->z : INFINITY;
}

static bool ray_intersects_aabb(Ray* ray, AABB* aabb)
{
  float tmin = 0.0;
  float tmax = INFINITY;

  for (int i = 0; i < 3; i++)
  {
    bool sign  = signbit(ray->inverseDirection.val[i]);
    float bmin = aabb->boundaries[sign].val[i];
    float bmax = aabb->boundaries[!sign].val[i];

    float dmin = (bmin - ray->origin.val[i]) * ray->inverseDirection.val[i];
    float dmax = (bmax - ray->origin.val[i]) * ray->inverseDirection.val[i];

    tmin = fmaxf(dmin, tmin);
    tmax = fminf(dmax, tmax);
  }

  return tmin < tmax;
}

static bool ray_intersects_triangle(Ray* ray, Vec3* p0, Vec3* p1, Vec3* p2)
{
  Vec3 e1 = *p1;
  vec3_subtract(&e1, p0);

  Vec3 e2 = *p1;
  vec3_subtract(&e2, p0);

  Vec3 p = ray->direction;
  vec3_cross(&p, &e2);
  float det = vec3_dot(&e1, &p);
  if (det > -0.0001f && det < 0.0001f)
  {
    return false;
  }

  Vec3 t = ray->origin;
  vec3_subtract(&t, p0);

  float u = vec3_dot(&t, &p) / det;
  if (u < 0.0 || u > 1.0)
  {
    return false;
  }

  Vec3 q = t;
  vec3_cross(&q, &e1);
  float v = vec3_dot(&ray->direction, &q) / det;
  if (v < 0.0 || (u + v) > 1.0)
  {
    return false;
  }

  float r = vec3_dot(&e2, &q) / det;
  if (r < 0.0001f)
  {
    return false;
  }

  return true;
}

// bool ray_cast(Ray* ray, BoundingVolumeHierarchy* bvh, BoundingVolumeNode*
// node,
//     Vec3* hitPosition)
//{
//   if (!ray_intersects_aabb(ray, &node->bounds))
//   {
//     return false;
//   }
//
//   if (node->left == NULL || node->right == NULL)
//   {
//     for (size_t i = 0; i < node->numOfTris; i++)
//     {
//       Triangle* prim = &node->tris[i];
//       Vec3* p0       = &((Vec4*) auto_array_get(&bvh->vertices,
//       prim->a))->xyz; Vec3* p1       = &((Vec4*)
//       auto_array_get(&bvh->vertices, prim->b))->xyz; Vec3* p2       =
//       &((Vec4*) auto_array_get(&bvh->vertices, prim->c))->xyz;
//
//       if (ray_intersects_triangle(ray, p0, p1, p2))
//       {
//         return true;
//       }
//     }
//     return false;
//   }
//
//   if (ray_cast(ray, bvh, node->left, hitPosition))
//   {
//     return true;
//   }
//
//   if (ray_cast(ray, bvh, node->right, hitPosition))
//   {
//     return true;
//   }
//
//   return false;
// }

static __m128 horizontal_min(__m128 x)
{
  x = _mm_min_ps(x, _mm_shuffle_ps(x, x, _MM_SHUFFLE(0, 0, 3, 2)));
  x = _mm_min_ps(x, _mm_shuffle_ps(x, x, _MM_SHUFFLE(0, 0, 0, 1)));
  return x;
}

static __m128 horizontal_max(__m128 x)
{
  x = _mm_max_ps(x, _mm_shuffle_ps(x, x, _MM_SHUFFLE(0, 0, 3, 2)));
  x = _mm_max_ps(x, _mm_shuffle_ps(x, x, _MM_SHUFFLE(0, 0, 0, 1)));
  return x;
}

bool ray_cast(Ray* ray, BoundingVolumeHierarchy* bvh)
{
  // TODO: Start off by going down a few bvh layers.
  // Allocate initial nodes.
  AutoArray* nodes = malloc(sizeof(AutoArray));
  if (nodes == NULL)
  {
    return false;
  }
  auto_array_create(nodes, sizeof(BoundingVolumeNode*));
  *(BoundingVolumeNode**) auto_array_allocate(nodes) =
      (BoundingVolumeNode*) stable_auto_array_get(&bvh->nodes, 0);
  int numberNodesActive = 1;

  /*AutoArray boundaries;
  auto_array_create(&boundaries, sizeof(AABB*));*/

  // Calculate which nodes are intersecting with the ray.
  AutoArray nodeIntersects;
  auto_array_create(&nodeIntersects, sizeof(bool));
  (void) auto_array_allocate_many(&nodeIntersects, nodes->size);

  bool raySign[3]  = {signbit(ray->inverseDirection.val[0]),
       signbit(ray->inverseDirection.val[1]),
       signbit(ray->inverseDirection.val[2])};
  __m128 rayOrigin = _mm_maskz_loadu_ps(0b111, ray->origin.val);
  __m128 inverseDirection =
      _mm_maskz_loadu_ps(0b111, ray->inverseDirection.val);

  /*while (numberNodesActive > 0)
  {
    for (int i = 0; i < nodes->size; i++)
    {*/
  BoundingVolumeNode* node = *(BoundingVolumeNode**) auto_array_get(nodes, 0);

  __m128 boundaries[2] = {
      _mm_maskz_loadu_ps(0b111, node->bounds.boundaries[0].val),
      _mm_maskz_loadu_ps(0b111, node->bounds.boundaries[1].val)};

  __mmask8 rayMask = raySign[0] | (raySign[1] << 1) | (raySign[2] << 2);

  __m128 boundaryMin = _mm_mask_blend_ps(rayMask, boundaries[0], boundaries[1]);
  __m128 boundaryMax = _mm_mask_blend_ps(rayMask, boundaries[1], boundaries[0]);

  __m128 minimum =
      _mm_mul_ps(_mm_sub_ps(boundaryMin, rayOrigin), inverseDirection);
  minimum.m128_f32[3] = 0.0;
  __m128 maximum =
      _mm_mul_ps(_mm_sub_ps(boundaryMax, rayOrigin), inverseDirection);
  minimum.m128_f32[3] = INFINITY;

  float tMin;
  _mm_mask_store_ps(&tMin, 1, horizontal_max(minimum));
  float tMax;
  _mm_mask_store_ps(&tMax, 1, horizontal_min(maximum));

  //*(bool*) auto_array_get(&nodeIntersects, i) = tMin < tMax;
  /* }
 }*/
  free(nodes);

  return tMin < tMax;
}
