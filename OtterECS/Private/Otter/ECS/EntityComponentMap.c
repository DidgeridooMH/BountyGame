#include "Otter/ECS/EntityComponentMap.h"

void entity_component_map_create(EntityComponentMap* map)
{
  bit_map_create(&map->entityUsed);
  bit_map_create(&map->components);
}

void entity_component_map_destroy(EntityComponentMap* map)
{
  bit_map_destroy(&map->entityUsed);
  bit_map_destroy(&map->components);
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
    auto_array_pop_many(&map->components, compactedEntries);
  }
}

void entity_component_map_add_component(
    EntityComponentMap* map, uint64_t entity, uint64_t component)
{
  bit_map_set_bit(&map->components, entity, component, true);
}

void entity_component_map_delete_component(
    EntityComponentMap* map, uint64_t entity, uint64_t component)
{
  bit_map_set_bit(&map->components, entity, component, false);
}
