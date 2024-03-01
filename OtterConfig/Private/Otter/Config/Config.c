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

HashMap* config_parse(char* config)
{
  HashMap* map =
      hash_map_create(HASH_MAP_DEFAULT_BUCKETS, HASH_MAP_DEFAULT_COEF);
  if (map == NULL)
  {
    return NULL;
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
      fprintf(stderr, "Warning: Invalid key pair on line %d\n", lineNumber);
      continue;
    }
    line[separator] = '\0';

    char* key   = line;
    char* value = &line[separator + 1];

    if (!hash_map_set_value(map, key, value, strlen(value) + 1))
    {
      fprintf(stderr,
          "Something went horribly wrong with the HashMap. Oh its bad...\n");
      return NULL;
    }
  }

  return map;
}