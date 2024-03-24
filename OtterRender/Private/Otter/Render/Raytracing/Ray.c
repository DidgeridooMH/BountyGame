#include "Otter/Render/Raytracing/Ray.h"

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
  Vec2 tx = aabb->x;
  vec2_subtract_scalar(&tx, ray->origin.x);
  vec2_multiply(&tx, ray->inverseDirection.x);

  Vec2 ty = aabb->y;
  vec2_subtract_scalar(&ty, ray->origin.y);
  vec2_multiply(&ty, ray->inverseDirection.y);

  Vec2 tz = aabb->z;
  vec2_subtract_scalar(&tz, ray->origin.z);
  vec2_multiply(&tz, ray->inverseDirection.z);

  float tmin = fminf(tx.x, tx.y);
  tmin       = fmaxf(tmin, fminf(ty.x, ty.y));
  tmin       = fmaxf(tmin, fminf(tz.x, tz.y));

  float tmax = fmaxf(tx.x, tx.y);
  tmax       = fminf(tmax, fmaxf(ty.x, ty.y));
  tmax       = fminf(tmax, fmaxf(tz.x, tz.y));

  return tmax >= tmin;
}

bool ray_cast(Ray* ray, BoundingVolumeHierarchy* bvh, Vec3* hitPosition)
{
  BoundingVolumeNode* node = stable_auto_array_get(&bvh->nodes, 0);

  while (node->left != NULL && node->right != NULL)
  {
    if (node->right->numOfPrimitives > 0
        && ray_intersects_aabb(ray, &node->right->bounds))
    {
      node = node->right;
    }
    else if (node->left->numOfPrimitives > 0
             && ray_intersects_aabb(ray, &node->left->bounds))
    {
      node = node->left;
    }
    else
    {
      return false;
    }
  }

  // for (size_t i = 0; i < node->numOfPrimitives; i++)
  //{
  //   size_t prim   = node->primitives[i];
  //   MeshVertex* a = &bvh->vertices[bvh->indices[prim * 3]];
  //   MeshVertex* b = &bvh->vertices[bvh->indices[prim * 3 + 1]];
  //   MeshVertex* c = &bvh->vertices[bvh->indices[prim * 3 + 2]];

  //  Vec3 e1 = b->position;
  //  vec3_subtract(&e1, &a->position);
  //  Vec3 e2 = c->position;
  //  vec3_subtract(&e2, &a->position);

  //  Vec3 p = ray->direction;
  //  vec3_cross(&p, &e2);
  //  float det = vec3_dot(&e1, &p);
  //  if (det < 0.0f)
  //  {
  //    continue;
  //  }

  //  // return true;
  //}

  return true;
}
