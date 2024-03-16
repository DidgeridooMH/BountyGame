#include "Otter/Util/HashMap.h"

bool hash_map_create(HashMap* map, size_t numOfBuckets, size_t coefficient)
{
  map->numOfBuckets = numOfBuckets;
  map->coefficient  = coefficient;

  map->buckets = malloc(numOfBuckets * sizeof(StableAutoArray));
  if (map->buckets == NULL)
  {
    return false;
  }

  for (int i = 0; i < numOfBuckets; i++)
  {
    stable_auto_array_create(
        &map->buckets[i], sizeof(KeyValue), SAA_DEFAULT_CHUNK_SIZE);
  }

  return true;
}

void hash_map_destroy(HashMap* map, HashMapDestroyFn destructor)
{
  for (size_t i = 0; i < map->numOfBuckets; i++)
  {
    for (uint32_t e = 0; e < map->buckets[i].size; e++)
    {
      KeyValue* keyValue = stable_auto_array_get(&map->buckets[i], e);
      free(keyValue->key);

      if (destructor != NULL)
      {
        destructor(keyValue->value);
      }
    }

    stable_auto_array_destroy(&map->buckets[i]);
  }
  free(map->buckets);
}

static StableAutoArray* hash_map_get_bucket(HashMap* map, const char* key)
{
  size_t hash = 0;
  while (*key != '\0')
  {
    hash = ((hash << 2) + *key) * map->coefficient;
    key++;
  }
  return &map->buckets[hash % map->numOfBuckets];
}

bool hash_map_set_value(HashMap* map, const char* key, void* value)
{
  StableAutoArray* bucket = hash_map_get_bucket(map, key);
  KeyValue* keyValue      = stable_auto_array_allocate(bucket);
  if (keyValue == NULL)
  {
    fprintf(stderr, "OOM\n");
    return false;
  }

  keyValue->key = _strdup(key);
  if (keyValue->key == NULL)
  {
    return false;
  }

  keyValue->value = value;

  return true;
}

void* hash_map_get_value(HashMap* map, const char* key)
{
  StableAutoArray* bucket = hash_map_get_bucket(map, key);
  for (uint32_t i = 0; i < bucket->size; i++)
  {
    KeyValue* keyValue = stable_auto_array_get(bucket, i);
    if (strcmp(keyValue->key, key) == 0)
    {
      return keyValue->value;
    }
  }
  return NULL;
}
