#include "Otter/ECS/EntityComponentMap.h"

#include "Otter/Util/Array/SparseAutoArray.h"
#include "Otter/Util/HashMap.h"
#include "Otter/Util/Log.h"

void entity_component_map_create(EntityComponentMap* map)
{
  sparse_auto_array_create(&map->entities, sizeof(HashMap));
  bit_map_create(&map->components);
  component_pool_create(&map->componentPool);
}

static void entity_component_map_destroy_all_components(
    EntityComponentMap* map, ComponentPool* componentPool)
{
  for (uint64_t i = 0; i < map->entities.components.size; i++)
  {
    if (bit_map_get(&map->entities.usedMask, i))
    {
      entity_component_map_destroy_entity(map, i);
    }
  }
}

void entity_component_map_destroy(EntityComponentMap* map)
{
  entity_component_map_destroy_all_components(map, &map->componentPool);
  bit_map_destroy(&map->components);
  sparse_auto_array_destroy(&map->entities);
}

uint64_t entity_component_map_create_entity(EntityComponentMap* map)
{
  uint64_t index = sparse_auto_array_allocate(&map->entities);
  HashMap* componentMap =
      (HashMap*) sparse_auto_array_get(&map->entities, index);
  if (!hash_map_create(
          componentMap, HASH_MAP_DEFAULT_BUCKETS, HASH_MAP_DEFAULT_COEF))
  {
    // TODO: Handle error.
    LOG_ERROR("Unable to create component map for entity.");
    exit(-1);
  }

  while (map->components.size <= index)
  {
    bit_map_expand(&map->components);
  }

  return index;
}

void entity_component_map_destroy_entity(
    EntityComponentMap* map, uint64_t entity)
{
  HashMap* componentMap =
      (HashMap*) sparse_auto_array_get(&map->entities, entity);
  for (uint64_t i = 0; i < BIT_MAP_MASK_ENTRY_SIZE; i++)
  {
    if (bit_map_get_bit(&map->components, entity, i))
    {
      uint64_t componentId =
          (uint64_t) hash_map_get_value(componentMap, &i, sizeof(uint64_t));
      component_pool_deallocate_component(&map->componentPool, i, componentId);
    }
  }
  hash_map_destroy(componentMap, NULL);

  uint64_t compactedEntries =
      sparse_auto_array_deallocate(&map->entities, entity);

  if (compactedEntries > 0)
  {
    auto_array_pop_many(&map->components, compactedEntries);
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
      (HashMap*) sparse_auto_array_get(&map->entities, entity);
  hash_map_set_value(
      componentMap, &component, sizeof(uint64_t), (void*) componentId);

  return componentId;
}

void entity_component_map_delete_component(
    EntityComponentMap* map, uint64_t entity, uint64_t component)
{
  bit_map_set_bit(&map->components, entity, component, false);

  HashMap* componentMap =
      (HashMap*) sparse_auto_array_get(&map->entities, entity);
  uint64_t componentId =
      (uint64_t) hash_map_get_value(componentMap, &component, sizeof(uint64_t));
  component_pool_deallocate_component(
      &map->componentPool, component, componentId);
}

void* entity_component_map_get_component(
    EntityComponentMap* map, uint64_t entity, uint64_t component)
{
  HashMap* componentMap =
      (HashMap*) sparse_auto_array_get(&map->entities, entity);
  uint64_t componentId =
      (uint64_t) hash_map_get_value(componentMap, &component, sizeof(uint64_t));
  return component_pool_get_component(
      &map->componentPool, component, componentId);
}
