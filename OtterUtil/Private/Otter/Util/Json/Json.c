#include "Otter/Util/Json/Json.h"

#include "Otter/Util/Json/JsonArray.h"
#include "Otter/Util/Json/JsonObject.h"

bool json_get_token(JsonToken* token, const char* document,
    size_t documentLength, size_t* const cursor)
{
  while (*cursor < documentLength && isspace(document[*cursor]))
  {
    *cursor += 1;
  }

  if (*cursor >= documentLength)
  {
    token->type = JTT_ERROR;
    return false;
  }

  token->tokenString       = NULL;
  token->tokenStringLength = 0;

  char tokenStart = document[*cursor];
  *cursor += 1;
  if (tokenStart == '{')
  {
    token->type = JTT_LDRAGON;
  }
  else if (tokenStart == '}')
  {
    token->type = JTT_RDRAGON;
  }
  else if (tokenStart == '[')
  {
    token->type = JTT_LBRACKET;
  }
  else if (tokenStart == ']')
  {
    token->type = JTT_RBRACKET;
  }
  else if (tokenStart == ':')
  {
    token->type = JTT_COLON;
  }
  else if (tokenStart == ',')
  {
    token->type = JTT_COMMA;
  }
  else if (tokenStart == '\"')
  {
    token->type        = JTT_STRING;
    token->tokenString = &document[*cursor];
    while (*cursor < documentLength && document[*cursor] != '\"')
    {
      *cursor += 1;
      token->tokenStringLength += 1;
    }
    *cursor += 1;
    if (*cursor >= documentLength)
    {
      token->type = JTT_ERROR;
      return false;
    }
  }
  else if (tokenStart == '-' || isdigit(tokenStart))
  {
    token->type       = JTT_NUMBER;
    char* endOfNumber = NULL;
    token->tokenFloat = strtof(&document[*cursor - 1], &endOfNumber);
    *cursor += (endOfNumber - &document[*cursor - 1]) - 1;
  }
  else if (strcmp(&document[*cursor - 1], "true") == 0)
  {
    token->type = JTT_TRUE;
    *cursor += strlen("true") - 1;
  }
  else if (strcmp(&document[*cursor - 1], "false") == 0)
  {
    token->type = JTT_FALSE;
    *cursor += strlen("false") - 1;
  }
  else if (strcmp(&document[*cursor - 1], "null") == 0)
  {
    token->type = JTT_NULL;
    *cursor += strlen("null") - 1;
  }
  else
  {
    token->type = JTT_ERROR;
    return false;
  }

  return true;
}

void json_peek_token(JsonToken* token, const char* document,
    size_t documentLength, size_t cursor)
{
  while (cursor < documentLength && isspace(document[cursor]))
  {
    cursor += 1;
  }

  if (cursor >= documentLength)
  {
    token->type = JTT_ERROR;
    return;
  }

  char tokenStart = document[cursor];
  if (tokenStart == '{')
  {
    token->type = JTT_LDRAGON;
  }
  else if (tokenStart == '}')
  {
    token->type = JTT_RDRAGON;
  }
  else if (tokenStart == '[')
  {
    token->type = JTT_LBRACKET;
  }
  else if (tokenStart == ']')
  {
    token->type = JTT_RBRACKET;
  }
  else if (tokenStart == ':')
  {
    token->type = JTT_COLON;
  }
  else if (tokenStart == ',')
  {
    token->type = JTT_COMMA;
  }
  else if (tokenStart == '\"')
  {
    token->type = JTT_STRING;
  }
  else if (tokenStart == '-' || isdigit(tokenStart))
  {
    token->type = JTT_NUMBER;
  }
  else if (strcmp(&document[cursor - 1], "true") == 0)
  {
    token->type = JTT_TRUE;
  }
  else if (strcmp(&document[cursor - 1], "false") == 0)
  {
    token->type = JTT_FALSE;
  }
  else if (strcmp(&document[cursor - 1], "null") == 0)
  {
    token->type = JTT_NULL;
  }
  else
  {
    token->type = JTT_ERROR;
  }
}

JsonValue* json_parse(
    const char* document, size_t documentLength, size_t* const cursor)
{
  JsonToken token;
  if (!json_get_token(&token, document, documentLength, cursor))
  {
    return NULL;
  }

  switch (token.type)
  {
  case JTT_LDRAGON:
    return json_parse_object_value(document, documentLength, cursor);
  case JTT_LBRACKET:
    return json_parse_array_value(document, documentLength, cursor);
  case JTT_STRING:
    {
      JsonValue* stringValue = malloc(sizeof(JsonValue));
      if (stringValue == NULL)
      {
        return NULL;
      }
      stringValue->type   = JT_STRING;
      stringValue->string = malloc(token.tokenStringLength + 1);
      if (stringValue->string == NULL)
      {
        return NULL;
      }
      strncpy_s(stringValue->string, token.tokenStringLength + 1,
          token.tokenString, token.tokenStringLength);
      return stringValue;
    }
  case JTT_NUMBER:
    {
      JsonValue* numberValue = malloc(sizeof(JsonValue));
      if (numberValue == NULL)
      {
        return NULL;
      }
      numberValue->type   = JT_NUMBER;
      numberValue->number = token.tokenFloat;
      return numberValue;
    }
  case JTT_TRUE:
    {
      JsonValue* boolValue = malloc(sizeof(JsonValue));
      if (boolValue == NULL)
      {
        return NULL;
      }
      boolValue->type    = JT_BOOLEAN;
      boolValue->boolean = true;
      return boolValue;
    }
  case JTT_FALSE:
    {
      JsonValue* boolValue = malloc(sizeof(JsonValue));
      if (boolValue == NULL)
      {
        return NULL;
      }
      boolValue->type    = JT_BOOLEAN;
      boolValue->boolean = false;
      return boolValue;
    }
  case JTT_NULL:
    return NULL;
  default:
    break;
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
    free(value->string);
    break;
  default:
    break;
  }

  free(value);
}
