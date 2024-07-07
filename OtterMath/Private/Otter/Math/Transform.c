#include "Otter/Math/Transform.h"

#include "Otter/Math/Mat.h"

void transform_identity(Transform* transform)
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

void transform_apply(Mat4 matrix, Transform* transform)
{
  mat4_scale(
      matrix, transform->scale.x, transform->scale.y, transform->scale.z);
  mat4_rotate(matrix, transform->rotation.x, transform->rotation.y,
      transform->rotation.z);
  mat4_translate(matrix, transform->position.x, transform->position.y,
      transform->position.z);
}
