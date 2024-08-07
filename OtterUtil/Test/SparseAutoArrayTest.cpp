extern "C"
{
#include "Otter/Util/Array/SparseAutoArray.h"
}

#include <gtest/gtest.h>

struct TestComponent
{
  int param1;
  int param2;
};

TEST(SparseAutoArrayTests, sparse_auto_array_create)
{
  SparseAutoArray list;
  sparse_auto_array_create(&list, sizeof(TestComponent));

  ASSERT_EQ(list.usedMask.size, 0);
  ASSERT_EQ(list.components.size, 0);

  sparse_auto_array_destroy(&list);
}

TEST(SparseAutoArrayTests, sparse_auto_array_allocate)
{
  SparseAutoArray list;
  sparse_auto_array_create(&list, sizeof(TestComponent));

  TestComponent* component = (TestComponent*) sparse_auto_array_allocate(&list);

  ASSERT_EQ(list.usedMask.size, 1);
  ASSERT_EQ(list.components.size, BIT_MAP_MASK_ENTRY_SIZE);

  sparse_auto_array_destroy(&list);
}

TEST(SparseAutoArrayTests, sparse_auto_array_allocate_many)
{
  SparseAutoArray list;
  sparse_auto_array_create(&list, sizeof(TestComponent));

  for (int i = 0; i < 3 * BIT_MAP_MASK_ENTRY_SIZE; i++)
  {
    sparse_auto_array_allocate(&list);
  }

  ASSERT_EQ(list.usedMask.size, 3);
  ASSERT_EQ(*(uint64_t*) auto_array_get(&list.usedMask, 0), ~0);
  ASSERT_EQ(*(uint64_t*) auto_array_get(&list.usedMask, 1), ~0);
  ASSERT_EQ(*(uint64_t*) auto_array_get(&list.usedMask, 2), ~0);
  ASSERT_EQ(list.components.size, 3 * BIT_MAP_MASK_ENTRY_SIZE);

  sparse_auto_array_destroy(&list);
}

TEST(SparseAutoArrayTests, sparse_auto_array_deallocate)
{
  SparseAutoArray list;
  sparse_auto_array_create(&list, sizeof(TestComponent));

  TestComponent* component1 =
      (TestComponent*) sparse_auto_array_allocate(&list);
  TestComponent* component2 =
      (TestComponent*) sparse_auto_array_allocate(&list);

  sparse_auto_array_deallocate(&list, 0);

  ASSERT_EQ(list.usedMask.size, 1);
  ASSERT_EQ(*(uint64_t*) auto_array_get(&list.usedMask, 0), 0b10);
  ASSERT_EQ(list.components.size, BIT_MAP_MASK_ENTRY_SIZE);

  sparse_auto_array_destroy(&list);
}

TEST(SparseAutoArrayTests, sparse_auto_array_deallocate_many)
{
  SparseAutoArray list;
  sparse_auto_array_create(&list, sizeof(TestComponent));

  TestComponent* component1 =
      (TestComponent*) sparse_auto_array_allocate(&list);
  TestComponent* component2 =
      (TestComponent*) sparse_auto_array_allocate(&list);

  sparse_auto_array_deallocate(&list, 1);
  sparse_auto_array_deallocate(&list, 0);

  ASSERT_EQ(list.usedMask.size, 0);
  ASSERT_EQ(list.components.size, 0);

  sparse_auto_array_destroy(&list);
}

TEST(SparseAutoArrayTests, sparse_auto_array_deallocate_second_mask)
{
  SparseAutoArray list;
  sparse_auto_array_create(&list, sizeof(TestComponent));

  for (int i = 0; i < 2 * BIT_MAP_MASK_ENTRY_SIZE; i++)
  {
    sparse_auto_array_allocate(&list);
  }

  sparse_auto_array_deallocate(&list, 0);
  sparse_auto_array_deallocate(&list, 63);
  sparse_auto_array_deallocate(&list, 64);

  ASSERT_EQ(list.usedMask.size, 2);
  ASSERT_EQ(*(uint64_t*) auto_array_get(&list.usedMask, 0),
      ~((1ULL << 63ULL) | 0b1ULL));
  ASSERT_EQ(*(uint64_t*) auto_array_get(&list.usedMask, 1), ~1ULL);
  ASSERT_EQ(list.components.size, 2 * BIT_MAP_MASK_ENTRY_SIZE);

  sparse_auto_array_destroy(&list);
}

TEST(SparseAutoArrayTests, sparse_auto_array_get)
{
  SparseAutoArray list;
  sparse_auto_array_create(&list, sizeof(TestComponent));

  sparse_auto_array_allocate(&list);
  sparse_auto_array_allocate(&list);

  TestComponent* component1 = (TestComponent*) sparse_auto_array_get(&list, 0);
  TestComponent* component2 = (TestComponent*) sparse_auto_array_get(&list, 1);

  ASSERT_NE(component1, nullptr);
  ASSERT_NE(component2, nullptr);

  sparse_auto_array_destroy(&list);
}

TEST(SparseAutoArrayTests, sparse_auto_array_get_not_allocated)
{
  SparseAutoArray list;
  sparse_auto_array_create(&list, sizeof(TestComponent));

  TestComponent* component = (TestComponent*) sparse_auto_array_get(&list, 0);

  ASSERT_EQ(component, nullptr);

  sparse_auto_array_destroy(&list);
}
