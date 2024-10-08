#include "Otter/Math/Vec.h"

#include "Otter/Math/Mat.h"

void vec4_multiply_mat4(Vec4* vec, Mat4 mat)
{
  Vec4 result = *vec;

  result.x = vec->x * mat[0][0] + vec->y * mat[1][0] + vec->z * mat[2][0]
           + vec->w * mat[3][0];
  result.y = vec->x * mat[0][1] + vec->y * mat[1][1] + vec->z * mat[2][1]
           + vec->w * mat[3][1];
  result.z = vec->x * mat[0][2] + vec->y * mat[1][2] + vec->z * mat[2][2]
           + vec->w * mat[3][2];
  result.w = vec->x * mat[0][3] + vec->y * mat[1][3] + vec->z * mat[2][3]
           + vec->w * mat[3][3];

  *vec = result;
}

void vec3_add(Vec3* result, const Vec3* operand)
{
  result->x += operand->x;
  result->y += operand->y;
  result->z += operand->z;
}

void vec3_subtract(Vec3* result, const Vec3* operand)
{
  result->x -= operand->x;
  result->y -= operand->y;
  result->z -= operand->z;
}

void vec3_multiply(Vec3* result, float operand)
{
  result->x *= operand;
  result->y *= operand;
  result->z *= operand;
}

void vec3_divide(Vec3* result, float operand)
{
  vec3_multiply(result, 1.0f / operand);
}

void vec3_normalize(Vec3* result)
{
  float w = sqrtf(
      result->x * result->x + result->y * result->y + result->z * result->z);
  vec3_divide(result, w);
}

void vec3_cross(Vec3* result, const Vec3* operand)
{
  Vec3 res = {result->y * operand->z - result->z * operand->y,
      result->z * operand->x - result->x * operand->z,
      result->x * operand->y - result->y * operand->x};
  *result  = res;
}

float vec3_dot(const Vec3* result, const Vec3* operand)
{
  return result->x * operand->x + result->y * operand->y
       + result->z * operand->z;
}

size_t vec3_max_index(const Vec3* result)
{
  size_t max = 0;
  if (result->y > result->x)
  {
    max = 1;
  }
  if (result->z > result->val[max])
  {
    max = 2;
  }
  return max;
}

void vec2_subtract_scalar(Vec2* result, float operand)
{
  result->x -= operand;
  result->y -= operand;
}

void vec2_multiply(Vec2* result, float operand)
{
  result->x *= operand;
  result->y *= operand;
}
