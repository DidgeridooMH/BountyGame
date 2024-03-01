#pragma once

#include "Otter/Config/export.h"

#define HASH_MAP_DEFAULT_BUCKETS 512
#define HASH_MAP_DEFAULT_COEF    769

typedef struct KeyValue
{
  char* key;
  void* value;
} KeyValue;

typedef struct HashBucket
{
  KeyValue* contents;
  size_t size;
} HashBucket;

typedef struct HashMap
{
  HashBucket* buckets;
  size_t numOfBuckets;
  size_t coefficient;
} HashMap;

OTTER_API HashMap* hash_map_create(size_t numOfBuckets, size_t coefficient);

OTTER_API void hash_map_destroy(HashMap* map);

OTTER_API bool hash_map_set_value(
    HashMap* map, const char* key, void* value, size_t valueSize);

OTTER_API void* hash_map_get_value(HashMap* map, const char* key);
