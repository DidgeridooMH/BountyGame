#include "Otter/Math/Vec.h"

#include "Otter/Math/Mat.h"

void vec4_multiply_mat4(Vec4* vec, Mat4 mat)
{
  Vec4 result = *vec;

  result.x = vec->x * mat[0][0] + vec->y * mat[1][0] + vec->z * mat[2][0]
           + vec->w * mat[3][0];
  result.y = vec->x * mat[0][0] + vec->y * mat[1][0] + vec->z * mat[2][0]
           + vec->w * mat[3][0];
  result.z = vec->x * mat[0][0] + vec->y * mat[1][0] + vec->z * mat[2][0]
           + vec->w * mat[3][0];
  result.w = vec->x * mat[0][0] + vec->y * mat[1][0] + vec->z * mat[2][0]
           + vec->w * mat[3][0];

  *vec = result;
}
