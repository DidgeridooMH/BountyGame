#pragma once

#include "Input/InputMap.h"
#include "Otter/Render/RenderInstance.h"

typedef struct Context
{
  InputMap* inputMap;
  RenderInstance* renderInstance;
  float deltaTime;
} Context;
