extern "C"
{
#include "Otter/Util/BitMap.h"
}

#include <gtest/gtest.h>

TEST(BitMapTest, CreateDestroy)
{
  BitMap map;
  bit_map_create(&map);
  bit_map_destroy(&map);
}

TEST(BitMapTest, SetGet)
{
  BitMap map;
  bit_map_create(&map);
  bit_map_expand(&map);

  bit_map_set(&map, 0, true);
  EXPECT_TRUE(bit_map_get(&map, 0));

  bit_map_set(&map, 0, false);
  EXPECT_FALSE(bit_map_get(&map, 0));

  bit_map_destroy(&map);
}

TEST(BitMapTest, SetGetMultiple)
{
  BitMap map;
  bit_map_create(&map);
  bit_map_expand(&map);
  bit_map_expand(&map);

  bit_map_set(&map, 0, false);
  bit_map_set(&map, 1, true);

  EXPECT_FALSE(bit_map_get(&map, 0));
  EXPECT_TRUE(bit_map_get(&map, 1));

  bit_map_destroy(&map);
}

TEST(BitMapTest, SetGetBit)
{
  BitMap map;
  bit_map_create(&map);
  bit_map_expand(&map);

  bit_map_set_bit(&map, 0, 0, true);
  EXPECT_TRUE(bit_map_get_bit(&map, 0, 0));

  bit_map_set_bit(&map, 0, 0, false);
  EXPECT_FALSE(bit_map_get_bit(&map, 0, 0));

  bit_map_destroy(&map);
}

TEST(BitMapTest, SetGetBitMultiple)
{
  BitMap map;
  bit_map_create(&map);
  bit_map_expand(&map);
  bit_map_expand(&map);

  bit_map_set_bit(&map, 0, 0, false);
  bit_map_set_bit(&map, 0, 1, true);
  bit_map_set_bit(&map, 1, 2, true);

  EXPECT_FALSE(bit_map_get_bit(&map, 0, 0));
  EXPECT_TRUE(bit_map_get_bit(&map, 0, 1));
  EXPECT_TRUE(bit_map_get_bit(&map, 1, 2));

  bit_map_destroy(&map);
}

TEST(BitMapTest, FindFirstUnsetEmpty)
{
  BitMap map;
  bit_map_create(&map);

  uint64_t index;
  EXPECT_FALSE(bit_map_find_first_unset(&map, &index));

  bit_map_destroy(&map);
}

TEST(BitMapTest, FindFirstUnsetFirst)
{
  BitMap map;
  bit_map_create(&map);
  bit_map_expand(&map);

  uint64_t index;
  EXPECT_TRUE(bit_map_find_first_unset(&map, &index));
  EXPECT_EQ(index, 0);

  bit_map_destroy(&map);
}

TEST(BitMapTest, FindFirstUnsetLastFirstSlot)
{
  BitMap map;
  bit_map_create(&map);
  bit_map_expand(&map);

  for (int i = 0; i < BIT_MAP_MASK_ENTRY_SIZE - 1; i++)
  {
    bit_map_set(&map, i, true);
  }

  uint64_t index;
  EXPECT_TRUE(bit_map_find_first_unset(&map, &index));
  EXPECT_EQ(index, BIT_MAP_MASK_ENTRY_SIZE - 1);

  bit_map_destroy(&map);
}

TEST(BitMapTest, FindFirstUnsetSecondSlot)
{
  BitMap map;
  bit_map_create(&map);
  bit_map_expand(&map);
  bit_map_expand(&map);

  for (int i = 0; i < BIT_MAP_MASK_ENTRY_SIZE; i++)
  {
    bit_map_set(&map, i, true);
  }

  uint64_t index;
  EXPECT_TRUE(bit_map_find_first_unset(&map, &index));
  EXPECT_EQ(index, BIT_MAP_MASK_ENTRY_SIZE);

  bit_map_destroy(&map);
}

TEST(BitMapTest, FindFirstUnsetSparse)
{
  BitMap map;
  bit_map_create(&map);
  bit_map_expand(&map);

  bit_map_set(&map, 0, true);
  bit_map_set(&map, 2, true);
  bit_map_set(&map, 4, true);
  bit_map_set(&map, 6, true);

  uint64_t index;
  EXPECT_TRUE(bit_map_find_first_unset(&map, &index));
  EXPECT_EQ(index, 1);

  bit_map_destroy(&map);
}

TEST(BitMapTest, Expand)
{
  BitMap map;
  bit_map_create(&map);

  EXPECT_EQ(bit_map_expand(&map), BIT_MAP_MASK_ENTRY_SIZE);
  EXPECT_EQ(map.size, 1);

  EXPECT_EQ(bit_map_expand(&map), BIT_MAP_MASK_ENTRY_SIZE);
  EXPECT_EQ(map.size, 2);

  bit_map_destroy(&map);
}

TEST(BitMapTest, Compact)
{
  BitMap map;
  bit_map_create(&map);
  bit_map_expand(&map);
  bit_map_expand(&map);

  for (int i = 0; i < BIT_MAP_MASK_ENTRY_SIZE; i++)
  {
    bit_map_set(&map, i, true);
  }

  EXPECT_EQ(bit_map_compact(&map), BIT_MAP_MASK_ENTRY_SIZE);
  EXPECT_EQ(map.size, 1);

  bit_map_destroy(&map);
}
