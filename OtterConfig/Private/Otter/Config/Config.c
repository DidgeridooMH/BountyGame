#include "Otter/Config/Config.h"

static char* config_get_line(char* config, size_t* cursor)
{
  char* line = &config[*cursor];
  while (config[*cursor] != '\n' && config[*cursor] != '\0')
  {
    *cursor += 1;
  }

  if (config[*cursor] == '\n')
  {
    config[*cursor] = '\0';
    *cursor += 1;
  }

  return line;
}

static size_t config_find_char(const char* s, char c)
{
  size_t index = 0;
  while (s[index] != '\0' && s[index] != c)
  {
    index++;
  }

  if (s[index] == '\0')
  {
    return SIZE_MAX;
  }

  return index;
}

static void config_trim(char* line)
{
  if (*line == '\0')
  {
    return;
  }

  char* start = line;
  while (*line != '\0')
  {
    line++;
  }
  while (--line != start && isspace(*line))
  {
    *line = '\0';
  }
}

bool config_parse(HashMap* configMap, char* config)
{
  if (!hash_map_create(
          configMap, HASH_MAP_DEFAULT_BUCKETS, HASH_MAP_DEFAULT_COEF))
  {
    return false;
  }

  int lineNumber = 0;
  size_t cursor  = 0;
  while (config[cursor] != '\0')
  {
    lineNumber += 1;
    char* line = config_get_line(config, &cursor);
    if (*line == '\0')
    {
      continue;
    }

    config_trim(line);

    size_t separator = config_find_char(line, '=');
    if (separator == SIZE_MAX)
    {
      LOG_WARNING("Invalid key pair on line %d", lineNumber);
      continue;
    }
    line[separator] = '\0';

    char* key = line;

    char* value = _strdup(&line[separator + 1]);
    if (value == NULL)
    {
      LOG_WARNING("Invalid value on line %d", lineNumber);
      continue;
    }

    if (!hash_map_set_value(configMap, key, strlen(key) + 1, value))
    {
      LOG_ERROR("Failed to set value for key %s", key);
      return false;
    }
  }

  return true;
}
