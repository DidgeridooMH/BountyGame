#include "Otter/ECS/ComponentPool.h"

#include "Otter/Util/Log.h"

void component_pool_create(ComponentPool* pool)
{
  pool->registeredComponents = 0;
}

void component_pool_destroy(ComponentPool* pool)
{
  for (uint64_t i = 0; i < _countof(pool->componentLists); ++i)
  {
    if (pool->registeredComponents & (1 << i))
    {
      sparse_auto_array_destroy(&pool->componentLists[i]);
    }
  }
}

void component_pool_register_component(
    ComponentPool* pool, uint64_t componentIndex, uint64_t componentSize)
{
  if (pool->registeredComponents & (1 << componentIndex))
  {
    LOG_WARNING("Component %llu is already registered", componentIndex);
    return;
  }

  pool->registeredComponents |= (1 << componentIndex);
  sparse_auto_array_create(
      &pool->componentLists[componentIndex], componentSize);
}

uint64_t component_pool_allocate_component(
    ComponentPool* pool, uint64_t componentIndex)
{
  if (!(pool->registeredComponents & (1 << componentIndex)))
  {
    LOG_WARNING("Component %llu is not registered", componentIndex);
    return COMPONENT_ID_INVALID;
  }

  return sparse_auto_array_allocate(&pool->componentLists[componentIndex]);
}

void component_pool_deallocate_component(
    ComponentPool* pool, uint64_t componentIndex, uint64_t component)
{
  if (!(pool->registeredComponents & (1 << componentIndex)))
  {
    LOG_WARNING("Component %llu is not registered", componentIndex);
    return;
  }

  sparse_auto_array_deallocate(
      &pool->componentLists[componentIndex], component);
}

void* component_pool_get_component(
    ComponentPool* pool, uint64_t componentIndex, uint64_t component)
{
  if (!(pool->registeredComponents & (1 << componentIndex)))
  {
    LOG_WARNING("Component %llu is not registered", componentIndex);
    return NULL;
  }

  return sparse_auto_array_get(
      &pool->componentLists[componentIndex], component);
}
