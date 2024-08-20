#include "Otter/ECS/EntityComponentMap.h"

#include "Otter/ECS/Entity.h"
#include "Otter/Util/Array/SparseAutoArray.h"
#include "Otter/Util/HashMap.h"
#include "Otter/Util/Log.h"

void entity_component_map_create(EntityComponentMap* map)
{
  sparse_auto_array_create(&map->entities, sizeof(Entity));
  bit_map_create(&map->components);
  component_pool_create(&map->componentPool);
}

static void entity_component_map_destroy_all_components(EntityComponentMap* map,
    ComponentPool* componentPool, ScriptEngine* scriptEngine)
{
  for (uint64_t i = 0; i < map->entities.components.size; i++)
  {
    if (bit_map_get(&map->entities.usedMask, i))
    {
      entity_component_map_destroy_entity(map, i, scriptEngine);
    }
  }
}

void entity_component_map_destroy(
    EntityComponentMap* map, ScriptEngine* scriptEngine)
{
  entity_component_map_destroy_all_components(
      map, &map->componentPool, scriptEngine);
  bit_map_destroy(&map->components);
  sparse_auto_array_destroy(&map->entities);
}

uint64_t entity_component_map_create_entity(EntityComponentMap* map)
{
  uint64_t index = sparse_auto_array_allocate(&map->entities);
  Entity* entity = (Entity*) sparse_auto_array_get(&map->entities, index);
  if (!entity_create(entity, index))
  {
    // TODO: Handle error.
    LOG_ERROR("Unable to create entity.");
    exit(-1);
  }

  while (map->components.size <= index)
  {
    bit_map_expand(&map->components);
  }

  return index;
}

void entity_component_map_destroy_entity(
    EntityComponentMap* map, uint64_t entityId, ScriptEngine* scriptEngine)
{
  Entity* entity = (Entity*) sparse_auto_array_get(&map->entities, entityId);
  for (uint64_t i = 0; i < BIT_MAP_MASK_ENTRY_SIZE; i++)
  {
    if (bit_map_get_bit(&map->components, entityId, i))
    {
      uint64_t componentId = (uint64_t) hash_map_get_value(
          &entity->componentIndices, &i, sizeof(uint64_t));
      component_pool_deallocate_component(&map->componentPool, i, componentId);
    }
  }

  entity_destroy(entity, scriptEngine);

  uint64_t compactedEntries =
      sparse_auto_array_deallocate(&map->entities, entityId);

  if (compactedEntries > 0)
  {
    auto_array_pop_many(&map->components, compactedEntries);
  }
}

uint64_t entity_component_map_add_component(
    EntityComponentMap* map, uint64_t entityId, uint64_t component)
{
  uint64_t componentId =
      component_pool_allocate_component(&map->componentPool, component);
  if (componentId == COMPONENT_ID_INVALID)
  {
    LOG_WARNING(
        "Unable to attach component %llu to entity %llu", component, entityId);
    return COMPONENT_ID_INVALID;
  }

  bit_map_set_bit(&map->components, entityId, component, true);

  Entity* entity = (Entity*) sparse_auto_array_get(&map->entities, entityId);
  hash_map_set_value(&entity->componentIndices, &component, sizeof(uint64_t),
      (void*) componentId);

  return componentId;
}

void entity_component_map_delete_component(
    EntityComponentMap* map, uint64_t entityId, uint64_t component)
{
  bit_map_set_bit(&map->components, entityId, component, false);

  Entity* entity = (Entity*) sparse_auto_array_get(&map->entities, entityId);
  uint64_t componentId = (uint64_t) hash_map_get_value(
      &entity->componentIndices, &component, sizeof(uint64_t));
  component_pool_deallocate_component(
      &map->componentPool, component, componentId);
}

Entity* entity_component_map_get_entity(
    EntityComponentMap* map, uint64_t entityId)
{
  Entity* entity = (Entity*) sparse_auto_array_get(&map->entities, entityId);
  if (entity == NULL)
  {
    LOG_WARNING("Unable to find entity %llu", entityId);
  }
  return entity;
}

void* entity_component_map_get_component(
    EntityComponentMap* map, uint64_t entityId, uint64_t component)
{
  Entity* entity = (Entity*) sparse_auto_array_get(&map->entities, entityId);
  uint64_t componentId = (uint64_t) hash_map_get_value(
      &entity->componentIndices, &component, sizeof(uint64_t));
  return component_pool_get_component(
      &map->componentPool, component, componentId);
}

void entity_component_map_run_scripts(
    EntityComponentMap* map, ScriptEngine* scriptEngine, void* context)
{
  for (uint64_t i = 0; i < map->entities.components.size; ++i)
  {
    if (bit_map_get(&map->entities.usedMask, i))
    {
      Entity* entity = (Entity*) sparse_auto_array_get(&map->entities, i);
      entity_run_update(entity, i, scriptEngine, context);
    }
  }
}
