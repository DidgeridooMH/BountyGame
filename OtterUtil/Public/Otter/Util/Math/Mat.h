#pragma once

#include "Otter/Util/Math/Vec.h"
#include "Otter/Util/export.h"

typedef Vec2 Mat2[2];
typedef Vec3 Mat3[3];
typedef Vec4 Mat4[4];

OTTERUTIL_API void mat4_identity(Mat4 matrix);
