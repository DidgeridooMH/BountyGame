#pragma once

#include "Otter/Render/Material.h"
#include "Otter/Render/Mesh.h"

typedef struct Asset
{
  Mesh* mesh;
  Material* material;
} Asset;
