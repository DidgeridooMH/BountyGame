#pragma once

#include "Otter/Math/Vec.h"
#include "Otter/Render/Raytracing/BoundingVolumeHierarchy.h"
#include "Otter/Render/export.h"

typedef struct Ray
{
  Vec3 origin;
  Vec3 direction;
  Vec3 inverseDirection;
} Ray;

OTTERRENDER_API void ray_create(
    Ray* ray, const Vec3* origin, const Vec3* direction);

OTTERRENDER_API bool ray_cast(Ray* ray, BoundingVolumeHierarchy* bvh);
