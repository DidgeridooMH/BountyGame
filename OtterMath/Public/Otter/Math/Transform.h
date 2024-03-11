#pragma once

#include "Otter/Math/Mat.h"
#include "Otter/Math/Vec.h"
#include "Otter/Math/export.h"

typedef struct Transform
{
  Vec3 position;
  Vec3 rotation;
  Vec3 scale;
} Transform;

OTTERMATH_API void transform_identity(Transform* transform);

OTTERMATH_API void transform_apply(Mat4 matrix, Transform* transform);
