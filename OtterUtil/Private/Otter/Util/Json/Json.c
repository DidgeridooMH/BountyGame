#include "Otter/Util/Json/Json.h"

#include "Otter/Util/Json/JsonArray.h"
#include "Otter/Util/Json/JsonNumber.h"
#include "Otter/Util/Json/JsonObject.h"
#include "Otter/Util/Json/JsonString.h"

// TODO: Check for end of document on isspace calls

JsonValue* json_parse(
    const char* document, size_t documentLength, size_t* const cursor)
{
  while (*cursor < documentLength)
  {
    if (isspace(document[*cursor]))
    {
      while (isspace(document[*cursor]))
      {
        *cursor += 1;
      }
    }
    else if (document[*cursor] == '{')
    {
      return json_parse_object_value(document, documentLength, cursor);
    }
    else if (document[*cursor] == '[')
    {
      return json_parse_array_value(document, documentLength, cursor);
    }
    else if (document[*cursor] == '\"')
    {
      return json_parse_string_value(document, documentLength, cursor);
    }
    else if (document[*cursor] == '-' || isdigit(document[*cursor]))
    {
      return json_parse_number_value(document, documentLength, cursor);
    }
    else if (strcmp(&document[*cursor], "true") == 0)
    {
      JsonValue* jsonBool = malloc(sizeof(JsonValue));
      if (jsonBool == NULL)
      {
        fprintf(stderr, "ERR: OOM\n");
        return NULL;
      }
      jsonBool->type    = JT_BOOLEAN;
      jsonBool->boolean = true;
      *cursor += strlen("true");
    }
    else if (strcmp(&document[*cursor], "false") == 0)
    {
      JsonValue* jsonBool = malloc(sizeof(JsonValue));
      if (jsonBool == NULL)
      {
        fprintf(stderr, "ERR: OOM\n");
        return NULL;
      }
      jsonBool->type    = JT_BOOLEAN;
      jsonBool->boolean = false;
      *cursor += strlen("false");
    }
    else if (strcmp(&document[*cursor], "null") == 0)
    {
      *cursor += strlen("null");
      return NULL;
    }
    else
    {
      break;
    }
  }

  return NULL;
}

void json_destroy(JsonValue* value)
{
  if (value == NULL)
  {
    return;
  }

  switch (value->type)
  {
  case JT_OBJECT:
    json_destroy_object(value);
    break;
  case JT_ARRAY:
    json_destroy_array(value);
    break;
  case JT_STRING:
    json_destroy_string_value(value);
    break;
  default:
    break;
  }

  free(value);
}
