#pragma once

#include "Otter/Math/Vec.h"
#include "Otter/Render/Memory/GpuBuffer.h"
#include "Otter/Render/export.h"

typedef struct MeshVertex
{
  Vec3 position;
  Vec3 normal;
  Vec3 tangent;
  Vec3 bitangent;
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
