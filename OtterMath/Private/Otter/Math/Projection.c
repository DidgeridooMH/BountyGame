#include "Otter/Math/Projection.h"

void projection_create_perspective(
    Mat4 matrix, float fov, float aspectRatio, float nearPlane, float farPlane)
{
  memset(matrix, 0, sizeof(Mat4));

  float fovRadians  = fov * (float) M_PI / 180.0f;
  float focalLength = 1.0f / tanf(fovRadians / 2.0f);

  float A = nearPlane / (farPlane - nearPlane);
  float B = farPlane * A;

  matrix[0][0] = focalLength;
  matrix[1][1] = focalLength * aspectRatio;
  matrix[2][2] = A;
  matrix[3][2] = B;
  matrix[2][3] = -1.0f;
}
