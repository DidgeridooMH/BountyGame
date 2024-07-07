#include "Otter/Util/Json/JsonArray.h"

#include "Otter/Util/Log.h"

JsonValue* json_parse_array_value(
    const char* document, size_t documentLength, size_t* const cursor)
{
  JsonValue* jsonArray = malloc(sizeof(JsonValue));
  if (jsonArray == NULL)
  {
    LOG_ERROR("Out of memory");
    return NULL;
  }
  jsonArray->type = JT_ARRAY;
  auto_array_create(&jsonArray->array, sizeof(JsonValue*));

  JsonToken token;
  do
  {
    json_peek_token(&token, document, documentLength, *cursor);
    if (token.type == JTT_RBRACKET)
    {
      json_get_token(&token, document, documentLength, cursor);
      break;
    }

    JsonValue** arrayElement = auto_array_allocate(&jsonArray->array);
    if (arrayElement == NULL)
    {
      json_destroy_array(jsonArray);
      return NULL;
    }
    *arrayElement = json_parse(document, documentLength, cursor);

    if (!json_get_token(&token, document, documentLength, cursor))
    {
      json_destroy_array(jsonArray);
      return NULL;
    }
  } while (token.type == JTT_COMMA);

  if (token.type != JTT_RBRACKET)
  {
    json_destroy_array(jsonArray);
    return NULL;
  }

  return jsonArray;
}

void json_destroy_array(JsonValue* value)
{
  for (uint32_t i = 0; i < value->array.size; i++)
  {
    json_destroy(*(JsonValue**) auto_array_get(&value->array, i));
  }
  auto_array_destroy(&value->array);
}
