#pragma once

#include "Otter/Math/MatDef.h"
#include "Otter/Math/Vec.h"
#include "Otter/Math/export.h"

OTTERMATH_API void mat4_identity(Mat4 matrix);

OTTERMATH_API void mat4_multiply(Mat4 operand, Mat4 matrix);

OTTERMATH_API void mat4_translate(Mat4 matrix, float x, float y, float z);

OTTERMATH_API void mat4_scale(Mat4 matrix, float x, float y, float z);

OTTERMATH_API void mat4_rotate(Mat4 matrix, float roll, float pitch, float yaw);
