#include "Otter/Config/HashMap.h"

HashMap* hash_map_create(size_t numOfBuckets, size_t coefficient)
{
  HashMap* map = malloc(sizeof(HashMap));
  if (map == NULL)
  {
    return NULL;
  }

  map->numOfBuckets = numOfBuckets;
  map->coefficient  = coefficient;

  map->buckets = calloc(numOfBuckets, sizeof(HashBucket));
  if (map->buckets == NULL)
  {
    free(map);
    return NULL;
  }

  return map;
}

void hash_map_destroy(HashMap* map)
{
  for (size_t i = 0; i < map->numOfBuckets; i++)
  {
    if (map->buckets[i].contents != NULL)
    {
      for (size_t j = 0; j < map->buckets[i].size; j++)
      {
        free(map->buckets[i].contents[j].key);
        free(map->buckets[i].contents[j].value);
      }
      free(map->buckets[i].contents);
    }
  }
  free(map->buckets);
  free(map);
}

static HashBucket* hash_map_get_bucket(HashMap* map, const char* key)
{
  size_t hash = 0;
  while (*key != '\0')
  {
    hash = ((hash << 2) + *key) * map->coefficient;
    key++;
  }
  return &map->buckets[hash % map->numOfBuckets];
}

bool hash_map_set_value(
    HashMap* map, const char* key, void* value, size_t valueSize)
{
  HashBucket* bucket = hash_map_get_bucket(map, key);
  bucket->size += 1;
  if (bucket->contents == NULL)
  {
    bucket->contents = malloc(sizeof(KeyValue));
  }
  else
  {
    bucket->contents =
        realloc(bucket->contents, sizeof(KeyValue) * bucket->size);
  }

  if (bucket->contents == NULL)
  {
    return false;
  }

  KeyValue* keyValue = &bucket->contents[bucket->size - 1];

  size_t keySize = strlen(key) + 1;
  keyValue->key  = malloc(keySize);
  if (keyValue->key == NULL)
  {
    return false;
  }
  strcpy_s(keyValue->key, keySize, key);

  keyValue->value = malloc(valueSize);
  if (keyValue->value == NULL)
  {
    return false;
  }
  memcpy(keyValue->value, value, valueSize);

  return true;
}

void* hash_map_get_value(HashMap* map, const char* key)
{
  HashBucket* bucket = hash_map_get_bucket(map, key);
  for (size_t i = 0; i < bucket->size; i++)
  {
    if (strcmp(bucket->contents[i].key, key) == 0)
    {
      return bucket->contents[i].value;
    }
  }
  return NULL;
}
