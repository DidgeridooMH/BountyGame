#pragma once

#include "Otter/Render/RenderPass/GBufferPass.h"
#include "Otter/Render/RenderPass/LightingPass.h"

typedef struct RenderStack
{
  GBufferPass gbufferPass;
  LightingPass lightingPass;
} RenderStack;

