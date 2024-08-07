extern "C"
{
#include "Otter/ECS/EntityComponentMap.h"

#include "Otter/ECS/ComponentPool.h"
}

#include <gtest/gtest.h>

TEST(EntityComponentMap, Create)
{
  EntityComponentMap map;
  entity_component_map_create(&map);
  entity_component_map_destroy(&map);
}

TEST(EntityComponentMap, CreateEntity)
{
  EntityComponentMap map;
  entity_component_map_create(&map);

  uint64_t entity = entity_component_map_create_entity(&map);
  EXPECT_EQ(entity, 0);

  entity_component_map_destroy(&map);
}

TEST(EntityComponentMap, DestroyEntity)
{
  EntityComponentMap map;
  entity_component_map_create(&map);

  uint64_t entity = entity_component_map_create_entity(&map);
  entity_component_map_destroy_entity(&map, entity);

  entity_component_map_destroy(&map);
}

TEST(EntityComponentMap, CreateEntityMultiple)
{
  EntityComponentMap map;
  entity_component_map_create(&map);

  uint64_t entity1 = entity_component_map_create_entity(&map);
  uint64_t entity2 = entity_component_map_create_entity(&map);
  uint64_t entity3 = entity_component_map_create_entity(&map);

  EXPECT_EQ(entity1, 0);
  EXPECT_EQ(entity2, 1);
  EXPECT_EQ(entity3, 2);

  entity_component_map_destroy(&map);
}

TEST(EntityComponentMap, CreateEntitySparse)
{
  EntityComponentMap map;
  entity_component_map_create(&map);

  uint64_t entity1 = entity_component_map_create_entity(&map);
  uint64_t entity2 = entity_component_map_create_entity(&map);
  uint64_t entity3 = entity_component_map_create_entity(&map);

  entity_component_map_destroy_entity(&map, entity2);

  uint64_t entity4 = entity_component_map_create_entity(&map);

  EXPECT_EQ(entity1, 0);
  EXPECT_EQ(entity2, 1);
  EXPECT_EQ(entity3, 2);
  EXPECT_EQ(entity4, 1);

  entity_component_map_destroy(&map);
}

TEST(EntityComponentMap, CreateEntitySparseMultiple)
{
  EntityComponentMap map;
  entity_component_map_create(&map);

  uint64_t entity1 = entity_component_map_create_entity(&map);
  uint64_t entity2 = entity_component_map_create_entity(&map);
  uint64_t entity3 = entity_component_map_create_entity(&map);

  entity_component_map_destroy_entity(&map, entity2);

  uint64_t entity4 = entity_component_map_create_entity(&map);
  uint64_t entity5 = entity_component_map_create_entity(&map);

  EXPECT_EQ(entity1, 0);
  EXPECT_EQ(entity2, 1);
  EXPECT_EQ(entity3, 2);
  EXPECT_EQ(entity4, 1);
  EXPECT_EQ(entity5, 3);

  entity_component_map_destroy(&map);
}

TEST(EntityComponentMap, DestroyEntityMultiple)
{
  EntityComponentMap map;
  entity_component_map_create(&map);

  uint64_t entity1 = entity_component_map_create_entity(&map);
  uint64_t entity2 = entity_component_map_create_entity(&map);
  uint64_t entity3 = entity_component_map_create_entity(&map);

  entity_component_map_destroy_entity(&map, entity1);
  entity_component_map_destroy_entity(&map, entity2);
  entity_component_map_destroy_entity(&map, entity3);

  entity_component_map_destroy(&map);
}

TEST(EntityComponentMap, DestroyEntitySparse)
{
  EntityComponentMap map;
  entity_component_map_create(&map);

  uint64_t entity1 = entity_component_map_create_entity(&map);
  uint64_t entity2 = entity_component_map_create_entity(&map);
  uint64_t entity3 = entity_component_map_create_entity(&map);

  entity_component_map_destroy_entity(&map, entity2);

  uint64_t entity4 = entity_component_map_create_entity(&map);

  entity_component_map_destroy_entity(&map, entity1);
  entity_component_map_destroy_entity(&map, entity3);
  entity_component_map_destroy_entity(&map, entity4);

  entity_component_map_destroy(&map);
}

#define COMPONENT_ID 0
struct Component
{
  uint32_t param1;
  uint32_t param2;
  uint32_t param3;
};

TEST(EntityComponentMap, AddComponent)
{
  EntityComponentMap map;
  entity_component_map_create(&map);

  component_pool_register_component(
      &map.componentPool, COMPONENT_ID, sizeof(Component));

  uint64_t entity = entity_component_map_create_entity(&map);

  uint64_t componentId =
      entity_component_map_add_component(&map, entity, COMPONENT_ID);

  Component* componentPtr = (Component*) component_pool_get_component(
      &map.componentPool, COMPONENT_ID, componentId);
  EXPECT_NE(componentPtr, nullptr);

  entity_component_map_destroy(&map);
}

TEST(EntityComponentMap, GetComponent)
{
  EntityComponentMap map;
  entity_component_map_create(&map);

  component_pool_register_component(
      &map.componentPool, COMPONENT_ID, sizeof(Component));

  uint64_t entity = entity_component_map_create_entity(&map);

  uint64_t componentId =
      entity_component_map_add_component(&map, entity, COMPONENT_ID);

  Component* componentPtr = (Component*) entity_component_map_get_component(
      &map, entity, COMPONENT_ID);
  EXPECT_NE(componentPtr, nullptr);

  entity_component_map_destroy(&map);
}
