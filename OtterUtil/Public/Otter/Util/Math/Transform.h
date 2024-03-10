#pragma once

#include "Otter/Util/Math/Mat.h"
#include "Otter/Util/Math/Vec.h"

typedef struct Transform
{
  Vec3 position;
  Vec3 rotation;
  Vec3 scale;
} Transform;

inline void transform_identity(Transform* transform)
{
  transform->position.x = 0;
  transform->position.y = 0;
  transform->position.z = 0;

  transform->rotation.x = 0;
  transform->rotation.y = 0;
  transform->rotation.z = 0;

  transform->scale.x = 1;
  transform->scale.y = 1;
  transform->scale.z = 1;
}

inline void transform_apply(Mat4 matrix, Transform* transform)
{
  mat4_scale(
      matrix, transform->scale.x, transform->scale.y, transform->scale.z);
  mat4_rotate(matrix, transform->rotation.x, transform->rotation.y,
      transform->rotation.z);
  mat4_translate(matrix, transform->position.x, transform->position.y,
      transform->position.z);
}
