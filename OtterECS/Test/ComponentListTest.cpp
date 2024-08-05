extern "C"
{
#include "Otter/ECS/ComponentList.h"
}

#include <gtest/gtest.h>

struct TestComponent
{
  int param1;
  int param2;
};

TEST(ComponentListTests, component_list_create)
{
  ComponentList list;
  component_list_create(&list, sizeof(TestComponent));

  ASSERT_EQ(list.usedMask.size, 0);
  ASSERT_EQ(list.components.size, 0);

  component_list_destroy(&list);
}

TEST(ComponentListTests, component_list_allocate)
{
  ComponentList list;
  component_list_create(&list, sizeof(TestComponent));

  TestComponent* component = (TestComponent*) component_list_allocate(&list);

  ASSERT_EQ(list.usedMask.size, 1);
  ASSERT_EQ(list.components.size, BIT_MAP_MASK_ENTRY_SIZE);

  component_list_destroy(&list);
}

TEST(ComponentListTests, component_list_allocate_many)
{
  ComponentList list;
  component_list_create(&list, sizeof(TestComponent));

  for (int i = 0; i < 3 * BIT_MAP_MASK_ENTRY_SIZE; i++)
  {
    component_list_allocate(&list);
  }

  ASSERT_EQ(list.usedMask.size, 3);
  ASSERT_EQ(*(uint64_t*) auto_array_get(&list.usedMask, 0), ~0);
  ASSERT_EQ(*(uint64_t*) auto_array_get(&list.usedMask, 1), ~0);
  ASSERT_EQ(*(uint64_t*) auto_array_get(&list.usedMask, 2), ~0);
  ASSERT_EQ(list.components.size, 3 * BIT_MAP_MASK_ENTRY_SIZE);

  component_list_destroy(&list);
}

TEST(ComponentListTests, component_list_deallocate)
{
  ComponentList list;
  component_list_create(&list, sizeof(TestComponent));

  TestComponent* component1 = (TestComponent*) component_list_allocate(&list);
  TestComponent* component2 = (TestComponent*) component_list_allocate(&list);

  component_list_deallocate(&list, 0);

  ASSERT_EQ(list.usedMask.size, 1);
  ASSERT_EQ(*(uint64_t*) auto_array_get(&list.usedMask, 0), 0b10);
  ASSERT_EQ(list.components.size, BIT_MAP_MASK_ENTRY_SIZE);

  component_list_destroy(&list);
}

TEST(ComponentListTests, component_list_deallocate_many)
{
  ComponentList list;
  component_list_create(&list, sizeof(TestComponent));

  TestComponent* component1 = (TestComponent*) component_list_allocate(&list);
  TestComponent* component2 = (TestComponent*) component_list_allocate(&list);

  component_list_deallocate(&list, 1);
  component_list_deallocate(&list, 0);

  ASSERT_EQ(list.usedMask.size, 0);
  ASSERT_EQ(list.components.size, 0);

  component_list_destroy(&list);
}

TEST(ComponentListTests, component_list_deallocate_second_mask)
{
  ComponentList list;
  component_list_create(&list, sizeof(TestComponent));

  for (int i = 0; i < 2 * BIT_MAP_MASK_ENTRY_SIZE; i++)
  {
    component_list_allocate(&list);
  }

  component_list_deallocate(&list, 0);
  component_list_deallocate(&list, 63);
  component_list_deallocate(&list, 64);

  ASSERT_EQ(list.usedMask.size, 2);
  ASSERT_EQ(*(uint64_t*) auto_array_get(&list.usedMask, 0),
      ~((1ULL << 63ULL) | 0b1ULL));
  ASSERT_EQ(*(uint64_t*) auto_array_get(&list.usedMask, 1), ~1ULL);
  ASSERT_EQ(list.components.size, 2 * BIT_MAP_MASK_ENTRY_SIZE);

  component_list_destroy(&list);
}

TEST(ComponentListTests, component_list_get)
{
  ComponentList list;
  component_list_create(&list, sizeof(TestComponent));

  component_list_allocate(&list);
  component_list_allocate(&list);

  TestComponent* component1 = (TestComponent*) component_list_get(&list, 0);
  TestComponent* component2 = (TestComponent*) component_list_get(&list, 1);

  ASSERT_NE(component1, nullptr);
  ASSERT_NE(component2, nullptr);

  component_list_destroy(&list);
}

TEST(ComponentListTests, component_list_get_not_allocated)
{
  ComponentList list;
  component_list_create(&list, sizeof(TestComponent));

  TestComponent* component = (TestComponent*) component_list_get(&list, 0);

  ASSERT_EQ(component, nullptr);

  component_list_destroy(&list);
}
