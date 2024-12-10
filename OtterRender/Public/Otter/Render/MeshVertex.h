#pragma once

#include "Otter/Math/Vec.h"

// TODO: When loading assets, it would be useful to use a paging system that
// allows an automated flow from Disk->RAM->VRAM. This would not only improve
// load times, but also eliminate the risk of running out of VRAM.
typedef struct MeshVertex
{
  Vec3 position;
  Vec3 normal;
  Vec4 tangent;
  Vec2 uv;
} MeshVertex;
