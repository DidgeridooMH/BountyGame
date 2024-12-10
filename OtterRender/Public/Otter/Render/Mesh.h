#pragma once

#include "Otter/Render/Memory/GpuBuffer.h"
#include "Otter/Render/export.h"

typedef struct Mesh
{
  GpuBuffer vertices;
  GpuBuffer indices;
} Mesh;

OTTERRENDER_API bool mesh_create(Mesh* mesh, const void* vertices,
    uint64_t vertexSize, uint64_t numOfVertices, const uint16_t indices[],
    uint64_t numOfIndices, VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice, VkCommandPool commandPool, VkQueue commandQueue);

OTTERRENDER_API void mesh_destroy(Mesh* mesh, VkDevice logicalDevice);

