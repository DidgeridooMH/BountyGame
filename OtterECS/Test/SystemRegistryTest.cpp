#include <gtest/gtest.h>

extern "C"
{
#include "Otter/ECS/EntityComponentMap.h"
#include "Otter/ECS/SystemRegistry.h"
}

enum Component
{
  POSITION,
  VELOCITY,
  SPIN
};

void test_system(void* context, uint64_t entity, void** components)
{
  (void) context;
  (void) entity;

  uint64_t* position = (uint64_t*) components[0];
  uint64_t* velocity = (uint64_t*) components[1];

  *position += *velocity;
}

TEST(SystemRegistry, Create)
{
  SystemRegistry registry;
  system_registry_create(&registry);
  system_registry_destroy(&registry);
}

TEST(SystemRegistry, RegisterSystem)
{
  SystemRegistry registry;
  system_registry_create(&registry);

  uint64_t system = system_registry_register_system(&registry, test_system, 0);
  EXPECT_EQ(system, 0);

  system_registry_destroy(&registry);
}

TEST(SystemRegistry, RegisterSystemMultiple)
{
  SystemRegistry registry;
  system_registry_create(&registry);

  uint64_t system1 = system_registry_register_system(&registry, test_system, 0);
  uint64_t system2 = system_registry_register_system(&registry, test_system, 0);
  uint64_t system3 = system_registry_register_system(&registry, test_system, 0);

  EXPECT_EQ(system1, 0);
  EXPECT_EQ(system2, 1);
  EXPECT_EQ(system3, 2);

  system_registry_destroy(&registry);
}

TEST(SystemRegistry, RegisterSystemWithComponents)
{
  SystemRegistry registry;
  system_registry_create(&registry);

  uint64_t system = system_registry_register_system(
      &registry, test_system, 2, POSITION, VELOCITY);
  EXPECT_EQ(system, 0);

  system_registry_destroy(&registry);
}

TEST(SystemRegistry, DeregisterSystem)
{
  SystemRegistry registry;
  system_registry_create(&registry);

  uint64_t system = system_registry_register_system(&registry, test_system, 0);
  system_registry_deregister_system(&registry, system);

  system_registry_destroy(&registry);
}

TEST(SystemRegistry, RunSystems)
{
  SystemRegistry registry;
  system_registry_create(&registry);

  uint64_t system = system_registry_register_system(
      &registry, test_system, 2, POSITION, VELOCITY);

  EntityComponentMap entityComponentMap;
  entity_component_map_create(&entityComponentMap);
  component_pool_register_component(
      &entityComponentMap.componentPool, POSITION, sizeof(uint64_t));
  component_pool_register_component(
      &entityComponentMap.componentPool, VELOCITY, sizeof(uint64_t));

  uint64_t entity = entity_component_map_create_entity(&entityComponentMap);

  entity_component_map_add_component(&entityComponentMap, entity, POSITION);
  uint64_t* position = (uint64_t*) entity_component_map_get_component(
      &entityComponentMap, entity, POSITION);
  *position = 10;

  entity_component_map_add_component(&entityComponentMap, entity, VELOCITY);
  uint64_t* velocity = (uint64_t*) entity_component_map_get_component(
      &entityComponentMap, entity, VELOCITY);
  *velocity = 5;

  system_registry_run_systems(&registry, &entityComponentMap, nullptr);

  EXPECT_EQ(*position, 15);
  EXPECT_EQ(*velocity, 5);

  entity_component_map_destroy(&entityComponentMap, NULL);
  system_registry_destroy(&registry);
}

TEST(SystemRegistry, RunSystemsMultiple)
{
  SystemRegistry registry;
  system_registry_create(&registry);

  uint64_t system1 = system_registry_register_system(
      &registry, test_system, 2, POSITION, VELOCITY);

  EntityComponentMap entityComponentMap;
  entity_component_map_create(&entityComponentMap);
  component_pool_register_component(
      &entityComponentMap.componentPool, POSITION, sizeof(uint64_t));
  component_pool_register_component(
      &entityComponentMap.componentPool, VELOCITY, sizeof(uint64_t));
  ;

  for (int i = 0; i < 10; i++)
  {
    uint64_t entity = entity_component_map_create_entity(&entityComponentMap);

    entity_component_map_add_component(&entityComponentMap, entity, POSITION);
    uint64_t* position = (uint64_t*) entity_component_map_get_component(
        &entityComponentMap, entity, POSITION);
    *position = 10;

    entity_component_map_add_component(&entityComponentMap, entity, VELOCITY);
    uint64_t* velocity = (uint64_t*) entity_component_map_get_component(
        &entityComponentMap, entity, VELOCITY);
    *velocity = 5;
  }

  system_registry_run_systems(&registry, &entityComponentMap, nullptr);

  for (int i = 0; i < 10; i++)
  {
    uint64_t entity = i;

    uint64_t* position = (uint64_t*) entity_component_map_get_component(
        &entityComponentMap, entity, POSITION);
    EXPECT_EQ(*position, 15);

    uint64_t* velocity = (uint64_t*) entity_component_map_get_component(
        &entityComponentMap, entity, VELOCITY);
    EXPECT_EQ(*velocity, 5);
  }

  entity_component_map_destroy(&entityComponentMap, NULL);
  system_registry_destroy(&registry);
}

TEST(SystemRegistry, RunSystemsInexactMatch)
{
  SystemRegistry registry;
  system_registry_create(&registry);

  uint64_t system = system_registry_register_system(
      &registry, test_system, 2, POSITION, VELOCITY);

  EntityComponentMap entityComponentMap;
  entity_component_map_create(&entityComponentMap);
  component_pool_register_component(
      &entityComponentMap.componentPool, POSITION, sizeof(uint64_t));
  component_pool_register_component(
      &entityComponentMap.componentPool, VELOCITY, sizeof(uint64_t));
  component_pool_register_component(
      &entityComponentMap.componentPool, SPIN, sizeof(uint64_t));

  uint64_t entity = entity_component_map_create_entity(&entityComponentMap);

  entity_component_map_add_component(&entityComponentMap, entity, POSITION);
  uint64_t* position = (uint64_t*) entity_component_map_get_component(
      &entityComponentMap, entity, POSITION);
  *position = 10;

  entity_component_map_add_component(&entityComponentMap, entity, VELOCITY);
  uint64_t* velocity = (uint64_t*) entity_component_map_get_component(
      &entityComponentMap, entity, VELOCITY);
  *velocity = 5;

  entity_component_map_add_component(&entityComponentMap, entity, SPIN);
  uint64_t* spin = (uint64_t*) entity_component_map_get_component(
      &entityComponentMap, entity, SPIN);
  *spin = 15;

  system_registry_run_systems(&registry, &entityComponentMap, nullptr);

  EXPECT_EQ(*position, 15);
  EXPECT_EQ(*velocity, 5);

  entity_component_map_destroy(&entityComponentMap, NULL);
  system_registry_destroy(&registry);
}

TEST(SystemRegistry, RunSystemsSparse)
{
  SystemRegistry registry;
  system_registry_create(&registry);

  uint64_t system = system_registry_register_system(
      &registry, test_system, 2, POSITION, SPIN);

  EntityComponentMap entityComponentMap;
  entity_component_map_create(&entityComponentMap);
  component_pool_register_component(
      &entityComponentMap.componentPool, POSITION, sizeof(uint64_t));
  component_pool_register_component(
      &entityComponentMap.componentPool, VELOCITY, sizeof(uint64_t));
  component_pool_register_component(
      &entityComponentMap.componentPool, SPIN, sizeof(uint64_t));

  uint64_t entity = entity_component_map_create_entity(&entityComponentMap);

  entity_component_map_add_component(&entityComponentMap, entity, POSITION);
  uint64_t* position = (uint64_t*) entity_component_map_get_component(
      &entityComponentMap, entity, POSITION);
  *position = 10;

  entity_component_map_add_component(&entityComponentMap, entity, SPIN);
  uint64_t* spin = (uint64_t*) entity_component_map_get_component(
      &entityComponentMap, entity, SPIN);
  *spin = 15;

  system_registry_run_systems(&registry, &entityComponentMap, nullptr);

  EXPECT_EQ(*position, 25);
  EXPECT_EQ(*spin, 15);

  entity_component_map_destroy(&entityComponentMap, NULL);
  system_registry_destroy(&registry);
}
