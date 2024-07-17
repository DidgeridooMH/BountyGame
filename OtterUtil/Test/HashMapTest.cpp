#include <gtest/gtest.h>

extern "C"
{
#include "Otter/Util/HashMap.h"
}

TEST(HashMapTest, CreateHashMap)
{
  HashMap map;
  bool result =
      hash_map_create(&map, HASH_MAP_DEFAULT_BUCKETS, HASH_MAP_DEFAULT_COEF);
  hash_map_destroy(&map, NULL);
  EXPECT_TRUE(result);
}

TEST(HashMapTest, IntKeyRetrieval)
{
  HashMap map;
  EXPECT_TRUE(
      hash_map_create(&map, HASH_MAP_DEFAULT_BUCKETS, HASH_MAP_DEFAULT_COEF));

  uint32_t key   = 23;
  uint32_t value = 42;
  EXPECT_TRUE(hash_map_set_value(&map, &key, sizeof(key), &value));

  uint32_t* result = (uint32_t*) hash_map_get_value(&map, &key, sizeof(key));
  EXPECT_EQ(*result, value);

  hash_map_destroy(&map, NULL);
}

TEST(HashMapTest, StringKeyRetrieval)
{
  HashMap map;
  EXPECT_TRUE(
      hash_map_create(&map, HASH_MAP_DEFAULT_BUCKETS, HASH_MAP_DEFAULT_COEF));

  constexpr const char* key = "superduperlongkeyorsomething";
  uint32_t value            = 42;
  EXPECT_TRUE(hash_map_set_value(&map, key, strlen(key), &value));

  uint32_t* result = (uint32_t*) hash_map_get_value(&map, key, strlen(key));
  EXPECT_EQ(*result, value);

  hash_map_destroy(&map, NULL);
}

TEST(HashMapTest, KeyNotFound)
{
  HashMap map;
  EXPECT_TRUE(
      hash_map_create(&map, HASH_MAP_DEFAULT_BUCKETS, HASH_MAP_DEFAULT_COEF));

  uint32_t key = 23;
  EXPECT_EQ(hash_map_get_value(&map, &key, sizeof(key)), nullptr);

  hash_map_destroy(&map, NULL);
}

TEST(HashMapTest, SetValueTwice)
{
  HashMap map;
  EXPECT_TRUE(
      hash_map_create(&map, HASH_MAP_DEFAULT_BUCKETS, HASH_MAP_DEFAULT_COEF));

  uint32_t key    = 23;
  uint32_t value1 = 42;
  uint32_t value2 = 43;
  EXPECT_TRUE(hash_map_set_value(&map, &key, sizeof(key), &value1));
  EXPECT_TRUE(hash_map_set_value(&map, &key, sizeof(key), &value2));

  uint32_t* result = (uint32_t*) hash_map_get_value(&map, &key, sizeof(key));
  EXPECT_EQ(*result, value2);

  hash_map_destroy(&map, NULL);
}

TEST(HashMapTest, CustomDestructor)
{
  HashMap map;
  EXPECT_TRUE(
      hash_map_create(&map, HASH_MAP_DEFAULT_BUCKETS, HASH_MAP_DEFAULT_COEF));

  uint32_t key   = 23;
  uint32_t value = 43;
  EXPECT_TRUE(hash_map_set_value(&map, &key, sizeof(key), &value));

  hash_map_destroy(&map, [](void* item) { *((uint32_t*) item) += 1; });

  EXPECT_EQ(value, 44);
}
