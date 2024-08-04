#pragma once

#include "Otter/ECS/BitMap.h"

typedef struct EntityComponentMap
{
  BitMap entityUsed;
  BitMap components;
} EntityComponentMap;

void entity_component_map_create(EntityComponentMap* map);

void entity_component_map_destroy(EntityComponentMap* map);

uint64_t entity_component_map_create_entity(EntityComponentMap* map);

void entity_component_map_destroy_entity(
    EntityComponentMap* map, uint64_t entity);

void entity_component_map_add_component(
    EntityComponentMap* map, uint64_t entity, uint64_t component);

void entity_component_map_delete_component(
    EntityComponentMap* map, uint64_t entity, uint64_t component);

