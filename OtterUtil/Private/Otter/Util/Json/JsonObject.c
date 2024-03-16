#include "Otter/Util/Json/JsonObject.h"

JsonValue* json_parse_object_value(
    const char* document, size_t documentLength, size_t* const cursor)
{
  JsonValue* jsonObject = malloc(sizeof(JsonValue));
  if (jsonObject == NULL)
  {
    fprintf(stderr, "ERR: OOM\n");
    return NULL;
  }
  jsonObject->type = JT_OBJECT;
  if (!hash_map_create(
          &jsonObject->object, HASH_MAP_DEFAULT_BUCKETS, HASH_MAP_DEFAULT_COEF))
  {
    free(jsonObject);
    return NULL;
  }

  JsonToken token;
  bool firstItem = true;
  while (json_get_token(&token, document, documentLength, cursor)
         && token.type != JTT_RDRAGON)
  {
    if (!firstItem)
    {
      if (token.type != JTT_COMMA
          || !json_get_token(&token, document, documentLength, cursor))
      {
        json_destroy_object(jsonObject);
        return NULL;
      }
    }
    firstItem = false;

    if (token.type != JTT_STRING)
    {
      json_destroy_object(jsonObject);
      return NULL;
    }
    char* key = malloc(token.tokenStringLength + 1);
    if (key == NULL)
    {
      json_destroy_object(jsonObject);
      return NULL;
    }
    strncpy_s(key, token.tokenStringLength + 1, token.tokenString,
        token.tokenStringLength);

    if (!json_get_token(&token, document, documentLength, cursor)
        || token.type != JTT_COLON)
    {
      json_destroy_object(jsonObject);
      free(key);
      return NULL;
    }

    JsonValue* value = json_parse(document, documentLength, cursor);
    if (value == NULL)
    {
      json_destroy_object(jsonObject);
      free(key);
      return NULL;
    }

    hash_map_set_value(&jsonObject->object, key, value);
    free(key);
  }

  if (token.type == JTT_ERROR)
  {
    json_destroy_object(jsonObject);
    return NULL;
  }

  return jsonObject;
}

void json_destroy_object(JsonValue* value)
{
  hash_map_destroy(&value->object, json_destroy);
}
