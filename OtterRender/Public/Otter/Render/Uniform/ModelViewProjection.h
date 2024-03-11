#pragma once

#include "Otter/Math/Mat.h"

typedef struct ModelViewProjection
{
  Mat4 model;
  Mat4 view;
  Mat4 projection;
} ModelViewProjection;
