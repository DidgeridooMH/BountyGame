#include "Otter/Util/Json/JsonObject.h"

#include "Otter/Util/Json/JsonString.h"

JsonValue* json_parse_object_value(
    const char* document, size_t documentLength, size_t* const cursor)
{
  *cursor += 1;
  JsonValue* jsonObject = malloc(sizeof(JsonValue));
  if (jsonObject == NULL)
  {
    fprintf(stderr, "ERR: OOM\n");
    return NULL;
  }
  jsonObject->type = JT_OBJECT;
  jsonObject->object =
      hash_map_create(HASH_MAP_DEFAULT_BUCKETS, HASH_MAP_DEFAULT_COEF);
  if (jsonObject->object == NULL)
  {
    free(jsonObject);
    return NULL;
  }

  while (isspace(document[*cursor]))
  {
    *cursor += 1;
  }
  bool firstItem = true;
  while (*cursor < documentLength && document[*cursor] != '}')
  {
    if (!firstItem)
    {
      if (document[*cursor] != ',')
      {
        json_destroy_object(jsonObject);
        return NULL;
      }
      *cursor += 1;

      while (isspace(document[*cursor]))
      {
        *cursor += 1;
      }
    }
    firstItem = false;

    char* key = json_parse_string(document, documentLength, cursor);
    if (key == NULL)
    {
      json_destroy_object(jsonObject);
      free(key);
      return NULL;
    }

    while (isspace(document[*cursor]))
    {
      *cursor += 1;
    }

    if (document[*cursor] != ':')
    {
      json_destroy_object(jsonObject);
      free(key);
      return NULL;
    }
    *cursor += 1;

    JsonValue* value = json_parse(document, documentLength, cursor);
    if (value == NULL)
    {
      json_destroy_object(jsonObject);
      free(key);
      return NULL;
    }

    hash_map_set_value(jsonObject->object, key, value);
    free(key);

    while (isspace(document[*cursor]))
    {
      *cursor += 1;
    }
  }
  return jsonObject;
}

void json_destroy_object(JsonValue* value)
{
  hash_map_destroy(value->object, json_destroy);
}
