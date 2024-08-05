#pragma once

#include "Otter/ECS/BitMap.h"
#include "Otter/ECS/ComponentPool.h"

typedef struct EntityComponentMap
{
  BitMap entityUsed;
  BitMap components;
  AutoArray entityComponents;
} EntityComponentMap;

OTTERECS_API void entity_component_map_create(EntityComponentMap* map);

OTTERECS_API void entity_component_map_destroy(EntityComponentMap* map);

OTTERECS_API uint64_t entity_component_map_create_entity(
    EntityComponentMap* map);

OTTERECS_API void entity_component_map_destroy_entity(
    EntityComponentMap* map, ComponentPool* componentPool, uint64_t entity);

OTTERECS_API uint64_t entity_component_map_add_component(
    EntityComponentMap* map, ComponentPool* componentPool, uint64_t entity,
    uint64_t component);

OTTERECS_API void entity_component_map_delete_component(EntityComponentMap* map,
    ComponentPool* componentPool, uint64_t entity, uint64_t component);
