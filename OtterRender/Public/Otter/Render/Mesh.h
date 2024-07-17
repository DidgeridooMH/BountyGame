#pragma once

#include "Otter/Math/Vec.h"
#include "Otter/Render/Memory/GpuBuffer.h"
#include "Otter/Render/export.h"

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

typedef struct Mesh
{
  GpuBuffer vertices;
  GpuBuffer indices;
} Mesh;

OTTERRENDER_API Mesh* mesh_create(const void* vertices, uint64_t vertexSize,
    uint64_t numOfVertices, const uint16_t indices[], uint64_t numOfIndices,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
    VkCommandPool commandPool, VkQueue commandQueue);

OTTERRENDER_API void mesh_destroy(Mesh* mesh, VkDevice logicalDevice);
