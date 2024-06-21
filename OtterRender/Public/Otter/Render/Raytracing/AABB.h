#pragma once

#include "Otter/Math/Vec.h"
#include "Otter/Render/export.h"

typedef struct AABB
{
  union
  {
    struct
    {
      Vec3 min;
      Vec3 max;
    };
    Vec3 boundaries[2];
  };
} AABB;

OTTERRENDER_API void aabb_initialize(AABB* bounds);

OTTERRENDER_API void aabb_adjust_bounds(AABB* bounds, Vec3* position);
