#include "Otter/ECS/EntityComponentMap.h"

#include "Otter/Util/HashMap.h"

void entity_component_map_create(EntityComponentMap* map)
{
  bit_map_create(&map->entityUsed);
  bit_map_create(&map->components);
  auto_array_create(&map->entityComponents, sizeof(HashMap));
  component_pool_create(&map->componentPool);
}

static void entity_component_map_destroy_all_components(
    EntityComponentMap* map, ComponentPool* componentPool)
{
  for (uint64_t i = 0; i < map->entityComponents.size; i++)
  {
    HashMap* componentMap =
        (HashMap*) auto_array_get(&map->entityComponents, i);
    for (uint64_t componentIndex = 0; componentIndex < BIT_MAP_MASK_ENTRY_SIZE;
         componentIndex++)
    {
      if (bit_map_get_bit(&map->components, i, componentIndex))
      {
        uint64_t componentId = (uint64_t) hash_map_get_value(
            componentMap, &componentIndex, sizeof(uint64_t));
        component_pool_deallocate_component(
            componentPool, componentIndex, componentId);
      }
    }
  }
}

void entity_component_map_destroy(EntityComponentMap* map)
{
  entity_component_map_destroy_all_components(map, &map->componentPool);

  bit_map_destroy(&map->entityUsed);
  bit_map_destroy(&map->components);
  for (uint64_t i = 0; i < map->entityComponents.size; i++)
  {
    HashMap* componentMap =
        (HashMap*) auto_array_get(&map->entityComponents, i);
    hash_map_destroy(componentMap, NULL);
  }
  auto_array_destroy(&map->entityComponents);
}

uint64_t entity_component_map_create_entity(EntityComponentMap* map)
{
  uint64_t index;
  if (!bit_map_find_first_unset(&map->entityUsed, &index))
  {
    index = map->components.size;
    bit_map_expand(&map->entityUsed);
    for (uint64_t i = 0; i < BIT_MAP_MASK_ENTRY_SIZE; i++)
    {
      bit_map_expand(&map->components);

      HashMap* componentMap =
          (HashMap*) auto_array_allocate(&map->entityComponents);
      if (!hash_map_create(
              componentMap, HASH_MAP_DEFAULT_BUCKETS, HASH_MAP_DEFAULT_COEF))
      {
        // TODO: Handle error.
        LOG_ERROR("Unable to create component map for entity.");
        exit(-1);
      }
    }
  }

  bit_map_set(&map->entityUsed, index, true);

  return index;
}

void entity_component_map_destroy_entity(
    EntityComponentMap* map, uint64_t entity)
{
  bit_map_set(&map->entityUsed, entity, false);

  uint64_t compactedEntries = bit_map_compact(&map->entityUsed);
  if (compactedEntries > 0)
  {
    for (uint64_t i = 0; i < compactedEntries; i++)
    {
      size_t entity = map->entityComponents.size - 1 - i;
      HashMap* componentMap =
          (HashMap*) auto_array_get(&map->entityComponents, entity);
      for (uint64_t componentIndex = 0;
           componentIndex < BIT_MAP_MASK_ENTRY_SIZE; componentIndex++)
      {
        if (bit_map_get_bit(&map->components, entity, componentIndex))
        {
          uint64_t componentId = (uint64_t) hash_map_get_value(
              componentMap, &componentIndex, sizeof(uint64_t));
          component_pool_deallocate_component(
              &map->componentPool, componentIndex, componentId);
        }
      }

      hash_map_destroy(componentMap, NULL);
    }
    auto_array_pop_many(&map->components, compactedEntries);
    auto_array_pop_many(&map->entityComponents, compactedEntries);
  }
}

uint64_t entity_component_map_add_component(
    EntityComponentMap* map, uint64_t entity, uint64_t component)
{
  uint64_t componentId =
      component_pool_allocate_component(&map->componentPool, component);
  if (componentId == COMPONENT_ID_INVALID)
  {
    LOG_WARNING(
        "Unable to attach component %llu to entity %llu", component, entity);
    return COMPONENT_ID_INVALID;
  }

  bit_map_set_bit(&map->components, entity, component, true);

  HashMap* componentMap =
      (HashMap*) auto_array_get(&map->entityComponents, entity);
  hash_map_set_value(
      componentMap, &component, sizeof(uint64_t), (void*) componentId);

  return componentId;
}

void entity_component_map_delete_component(
    EntityComponentMap* map, uint64_t entity, uint64_t component)
{
  bit_map_set_bit(&map->components, entity, component, false);

  HashMap* componentMap =
      (HashMap*) auto_array_get(&map->entityComponents, entity);
  uint64_t componentId =
      (uint64_t) hash_map_get_value(componentMap, &component, sizeof(uint64_t));
  component_pool_deallocate_component(
      &map->componentPool, component, componentId);
}

