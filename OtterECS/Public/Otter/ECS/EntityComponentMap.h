#pragma once

#include "Otter/ECS/ComponentPool.h"
#include "Otter/ECS/export.h"
#include "Otter/Util/Array/SparseAutoArray.h"
#include "Otter/Util/BitMap.h"

typedef struct EntityComponentMap
{
  SparseAutoArray entities;
  BitMap components;
  ComponentPool componentPool;
} EntityComponentMap;

OTTERECS_API void entity_component_map_create(EntityComponentMap* map);

OTTERECS_API void entity_component_map_destroy(EntityComponentMap* map);

OTTERECS_API uint64_t entity_component_map_create_entity(
    EntityComponentMap* map);

OTTERECS_API void entity_component_map_destroy_entity(
    EntityComponentMap* map, uint64_t entity);

OTTERECS_API uint64_t entity_component_map_add_component(
    EntityComponentMap* map, uint64_t entity, uint64_t component);

OTTERECS_API void entity_component_map_delete_component(
    EntityComponentMap* map, uint64_t entity, uint64_t component);

OTTERECS_API void* entity_component_map_get_component(
    EntityComponentMap* map, uint64_t entity, uint64_t component);

