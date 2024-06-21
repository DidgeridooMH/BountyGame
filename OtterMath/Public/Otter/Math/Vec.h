#pragma once

#include "Otter/Math/MatDef.h"
#include "Otter/Math/export.h"

typedef struct Vec2
{
  union
  {
    struct
    {
      float x;
      float y;
    };
    float val[2];
  };
} Vec2;

typedef struct Vec3
{
  union
  {
    struct
    {
      float x;
      float y;
      float z;
    };
    float val[3];
  };
} Vec3;

typedef struct Vec4
{
  union
  {
    float val[4];
    Vec3 xyz;
    struct
    {
      float x;
      float y;
      float z;
      float w;
    };
  };
} Vec4;

OTTERMATH_API void vec4_multiply_mat4(Vec4* vec, Mat4 mat);

OTTERMATH_API void vec3_add(Vec3* result, const Vec3* operand);

OTTERMATH_API void vec3_subtract(Vec3* result, const Vec3* operand);

OTTERMATH_API void vec3_multiply(Vec3* result, float operand);

OTTERMATH_API void vec3_divide(Vec3* result, float operand);

OTTERMATH_API void vec3_normalize(Vec3* result);

OTTERMATH_API void vec3_cross(Vec3* result, const Vec3* operand);

OTTERMATH_API float vec3_dot(const Vec3* result, const Vec3* operand);

OTTERMATH_API size_t vec3_max_index(const Vec3* result);

OTTERMATH_API void vec2_subtract_scalar(Vec2* result, float operand);

OTTERMATH_API void vec2_multiply(Vec2* result, float operand);
