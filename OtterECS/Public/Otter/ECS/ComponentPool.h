#pragma once

#include "Otter/ECS/ComponentList.h"
#include "Otter/ECS/export.h"

typedef struct ComponentPool
{
  ComponentList componentLists[64];
  uint64_t registeredComponents;
} ComponentPool;

OTTERECS_API void component_pool_create(ComponentPool* pool);

OTTERECS_API void component_pool_destroy(ComponentPool* pool);

OTTERECS_API void component_pool_register_component(
    ComponentPool* pool, uint64_t componentIndex, uint64_t componentSize);

OTTERECS_API uint64_t component_pool_allocate_component(
    ComponentPool* pool, uint64_t componentIndex);

OTTERECS_API void component_pool_deallocate_component(
    ComponentPool* pool, uint64_t componentIndex, uint64_t component);

OTTERECS_API void* component_pool_get_component(
    ComponentPool* pool, uint64_t componentIndex, uint64_t component);

