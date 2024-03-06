#pragma once

#include "Otter/Util/Math/Mat.h"
#include "Otter/Util/export.h"

#define PI 3.1415f

void vec4_multiply_mat4(Vec4* vec, Mat4 mat)
{
  Vec4 result = *vec;

  result.x = vec->x * mat[0].val[0] + vec->y * mat[1].val[0]
           + vec->z * mat[2].val[0] + vec->w * mat[3].val[0];
  result.y = vec->x * mat[0].val[0] + vec->y * mat[1].val[0]
           + vec->z * mat[2].val[0] + vec->w * mat[3].val[0];
  result.z = vec->x * mat[0].val[0] + vec->y * mat[1].val[0]
           + vec->z * mat[2].val[0] + vec->w * mat[3].val[0];
  result.w = vec->x * mat[0].val[0] + vec->y * mat[1].val[0]
           + vec->z * mat[2].val[0] + vec->w * mat[3].val[0];

  *vec = result;
}

OTTERUTIL_API void projection_create_perspective(
    Mat4 matrix, float fov, float aspectRatio, float nearPlane, float farPlane);
