#pragma once

#include "Otter/Util/StableAutoArray.h"
#include "Otter/Util/export.h"

#define HASH_MAP_DEFAULT_BUCKETS 512
#define HASH_MAP_DEFAULT_COEF    769

typedef struct KeyValue
{
  char* key;
  void* value;
} KeyValue;

typedef struct HashMap
{
  StableAutoArray* buckets;
  size_t numOfBuckets;
  size_t coefficient;
} HashMap;

typedef void (*HashMapDestroyFn)(void*);

OTTERUTIL_API bool hash_map_create(
    HashMap* map, size_t numOfBuckets, size_t coefficient);

OTTERUTIL_API void hash_map_destroy(HashMap* map, HashMapDestroyFn destructor);

OTTERUTIL_API bool hash_map_set_value(
    HashMap* map, const char* key, void* value);

OTTERUTIL_API void* hash_map_get_value(HashMap* map, const char* key);
