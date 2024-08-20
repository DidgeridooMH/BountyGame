#include "FreeCameraControls.h"

#include "Input/InputMap.h"
#include "Otter/Math/Mat.h"
#include "Otter/Render/RenderInstance.h"

void update_camera_position(
    InputMap* map, RenderInstance* renderInstance, float deltaTime)
{
  const float SPEED = 50.0f;

  float moveForward  = input_map_get_action_value(map, "move_forward");
  float moveBackward = input_map_get_action_value(map, "move_back");
  if (!isnan(moveForward) && !isnan(moveBackward))
  {
    Vec4 translation = {0.0f, 0.0f, 1.0f, 1.0f};
    Mat4 rotationMatrix;
    mat4_identity(rotationMatrix);
    mat4_rotate(rotationMatrix, renderInstance->cameraTransform.rotation.x,
        renderInstance->cameraTransform.rotation.y,
        renderInstance->cameraTransform.rotation.z);
    vec4_multiply_mat4(&translation, rotationMatrix);

    Vec3* translation3d = (Vec3*) &translation;
    vec3_multiply(
        translation3d, -(moveForward - moveBackward) * SPEED * deltaTime);
    vec3_add(&renderInstance->cameraTransform.position, (Vec3*) &translation);
  }

  float moveRight = input_map_get_action_value(map, "move_right");
  float moveLeft  = input_map_get_action_value(map, "move_left");
  if (!isnan(moveRight) && !isnan(moveLeft))
  {
    Vec4 translation = {1.0f, 0.0f, 0.0f, 1.0f};
    Mat4 rotationMatrix;
    mat4_identity(rotationMatrix);
    mat4_rotate(rotationMatrix, renderInstance->cameraTransform.rotation.x,
        renderInstance->cameraTransform.rotation.y,
        renderInstance->cameraTransform.rotation.z);
    vec4_multiply_mat4(&translation, rotationMatrix);

    Vec3* translation3d = (Vec3*) &translation;
    vec3_multiply(translation3d, -(moveLeft - moveRight) * SPEED * deltaTime);
    vec3_add(&renderInstance->cameraTransform.position, (Vec3*) &translation);
  }

  float turnLeft  = input_map_get_action_value(map, "turn_left");
  float turnRight = input_map_get_action_value(map, "turn_right");
  if (!isnan(turnLeft) && !isnan(turnRight))
  {
    renderInstance->cameraTransform.rotation.y +=
        (turnLeft - turnRight) * deltaTime * 3.f;
  }

  float moveUp   = input_map_get_action_value(map, "move_up");
  float moveDown = input_map_get_action_value(map, "move_down");
  if (!isnan(moveUp) && !isnan(moveDown))
  {
    renderInstance->cameraTransform.position.y -=
        (moveUp - moveDown) * deltaTime * 3.f;
  }
}
