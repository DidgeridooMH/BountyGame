#include "Otter/ECS/SystemRegistry.h"

#include <stdarg.h>

#include "Otter/ECS/EntityComponentMap.h"
#include "Otter/Util/Array/SparseAutoArray.h"
#include "Otter/Util/BitMap.h"

typedef struct System
{
  SystemCallback system;
  BitMapSlot componentMask;
  AutoArray components;
} System;

void system_registry_create(SystemRegistry* registry)
{
  sparse_auto_array_create(registry, sizeof(System));
}

void system_registry_destroy(SystemRegistry* registry)
{
  for (uint64_t i = 0; i < registry->components.size; ++i)
  {
    System* system = (System*) sparse_auto_array_get(registry, i);
    if (system != NULL)
    {
      auto_array_destroy(&system->components);
    }
  }
  sparse_auto_array_destroy(registry);
}

uint64_t system_registry_register_system(SystemRegistry* registry,
    SystemCallback systemCallback, int componentCount, ...)
{
  va_list args;
  va_start(args, componentCount);

  uint64_t id           = sparse_auto_array_allocate(registry);
  System* system        = (System*) sparse_auto_array_get(registry, id);
  system->system        = systemCallback;
  system->componentMask = 0;
  auto_array_create(&system->components, sizeof(uint64_t));

  for (int i = 0; i < componentCount; ++i)
  {
    uint64_t componentId = va_arg(args, uint64_t);
    system->componentMask |= (1 << componentId);
    *(uint64_t*) auto_array_allocate(&system->components) = componentId;
  }

  va_end(args);

  return id;
}

void system_registry_deregister_system(
    SystemRegistry* registry, uint64_t systemId)
{
  System* system = (System*) sparse_auto_array_get(registry, systemId);
  auto_array_destroy(&system->components);
  sparse_auto_array_deallocate(registry, systemId);
}

static void system_registry_run_system(
    System* system, EntityComponentMap* entityComponentMap, void* context)
{
  void* components[sizeof(BitMapSlot) * 8] = {0};
  uint64_t componentCount                  = 0;

  for (uint64_t entityId = 0;
       entityId < entityComponentMap->entities.components.size; ++entityId)
  {
    if (bit_map_get(&entityComponentMap->entities.usedMask, entityId))
    {
      // TODO: Precompute which entities have the required components so that we
      // have been cache locality. This will probably include sorting so that
      // components of high priority are at the front of the list.
      uint64_t componentMask =
          bit_map_get_slot(&entityComponentMap->components, entityId);
      if ((componentMask & system->componentMask) == system->componentMask)
      {
        for (uint64_t componentId = 0; componentId < BIT_MAP_MASK_ENTRY_SIZE;
             ++componentId)
        {
          if ((system->componentMask & (1ULL << componentId)) > 0)
          {
            components[componentCount++] = entity_component_map_get_component(
                entityComponentMap, entityId, componentId);
          }
        }

        system->system(context, entityId, components);
        componentCount = 0;
      }
    }
  }
}

void system_registry_run_systems(SystemRegistry* registry,
    EntityComponentMap* entityComponentMap, void* context)
{
  for (uint64_t i = 0; i < registry->components.size; ++i)
  {
    System* system = (System*) sparse_auto_array_get(registry, i);
    if (system != NULL)
    {
      system_registry_run_system(system, entityComponentMap, context);
    }
  }
}
