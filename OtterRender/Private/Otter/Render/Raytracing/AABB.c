#include "Otter/Render/Raytracing/AABB.h"

void aabb_initialize(AABB* bounds)
{
  bounds->min.x = INFINITY;
  bounds->min.y = INFINITY;
  bounds->min.z = INFINITY;

  bounds->max.x = -INFINITY;
  bounds->max.y = -INFINITY;
  bounds->max.z = -INFINITY;
}

void aabb_adjust_bounds(AABB* bounds, Vec3* position)
{
  bounds->min.x = fminf(position->x, bounds->min.x);
  bounds->min.y = fminf(position->y, bounds->min.y);
  bounds->min.z = fminf(position->z, bounds->min.z);

  bounds->max.x = fmaxf(position->x, bounds->max.z);
  bounds->max.y = fmaxf(position->y, bounds->max.y);
  bounds->max.z = fmaxf(position->z, bounds->max.z);
}
