#pragma once

#include "Otter/Math/Mat.h"
#include "Otter/Math/export.h"

OTTERMATH_API void projection_create_perspective(
    Mat4 matrix, float fov, float aspectRatio, float nearPlane, float farPlane);
