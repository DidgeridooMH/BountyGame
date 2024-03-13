#include "Otter/Util/Json/JsonArray.h"

JsonValue* json_parse_array_value(
    const char* document, size_t documentLength, size_t* const cursor)
{
  *cursor += 1;
  JsonValue* jsonArray = malloc(sizeof(JsonValue));
  if (jsonArray == NULL)
  {
    fprintf(stderr, "ERR: OOM\n");
    return NULL;
  }
  jsonArray->type = JT_ARRAY;
  auto_array_create(&jsonArray->array, sizeof(JsonValue*));

  while (isspace(document[*cursor]))
  {
    *cursor += 1;
  }
  while (*cursor < documentLength && document[*cursor] != ']')
  {
    while (isspace(document[*cursor]))
    {
      *cursor += 1;
    }
    if (jsonArray->array.size > 0)
    {
      if (document[*cursor] != ',')
      {
        json_destroy_array(jsonArray);
        return NULL;
      }
      *cursor += 1;
    }
    JsonValue** arrayElement = auto_array_allocate(&jsonArray->array);
    if (arrayElement == NULL)
    {
      json_destroy_array(jsonArray);
      return NULL;
    }
    *arrayElement = json_parse(document, documentLength, cursor);
    while (isspace(document[*cursor]))
    {
      *cursor += 1;
    }
  }

  if (*cursor >= documentLength)
  {
    json_destroy_array(jsonArray);
    return NULL;
  }
  *cursor += 1;

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
