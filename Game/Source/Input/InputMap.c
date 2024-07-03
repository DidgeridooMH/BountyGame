#include "Input/InputMap.h"

bool input_map_create(InputMap* map)
{
  return hash_map_create(&map->sourceToActions, HASH_MAP_DEFAULT_BUCKETS,
             HASH_MAP_DEFAULT_COEF)
      && hash_map_create(
          &map->actionValues, HASH_MAP_DEFAULT_BUCKETS, HASH_MAP_DEFAULT_COEF);
}

static void input_map_add_action_from_key_bind(
    char* action, size_t actionLength, char* input, InputMap* map)
{
  (void) actionLength;

  InputEventSource source;
  if (strlen(input) == 1)
  {
    source.source = INPUT_TYPE_KEYBOARD;
    source.index  = input[0];
    printf("KB(%c) -> %s\n", source.index, action);
  }
  else if (strlen(input) > 1 & input[0] == 'M')
  {
    source.source = INPUT_TYPE_MOUSE;
    source.index  = atoi(&input[1]);
    printf("M(%c) -> %s\n", source.index, action);
  }
  else if (strlen(input) > 1 & input[0] == 'C')
  {
    source.source = INPUT_TYPE_CONTROLLER;
    source.index  = atoi(&input[1]);
    printf("C(%c) -> %s\n", source.index, action);
  }
  else
  {
    fprintf(stderr, "Unknown input source %s for action %s\n", input, action);
    return;
  }

  input_map_add_action(map, source, action);
}

void input_map_load_key_binds(InputMap* map, HashMap* key_binds)
{
  hash_map_iterate(
      key_binds, (HashMapIterateFn) input_map_add_action_from_key_bind, map);
}

void input_map_add_action(InputMap* map, InputEventSource source, char* action)
{
  hash_map_set_value(&map->sourceToActions, &source, sizeof(InputEventSource),
      _strdup(action));
  hash_map_set_value_float(
      &map->actionValues, action, strlen(action) + 1, 0.0f);
}

void input_map_remove_action(InputMap* map, InputEventSource source)
{
  (void) map;
  (void) source;
  // TODO: This needs hashmap to have a remove function which in turn needs a
  // stable list to have a remove.
}

void input_map_update_actions(InputMap* map, AutoArray* inputs)
{
  for (size_t i = 0; i < inputs->size; ++i)
  {
    const InputEvent* event = auto_array_get(inputs, i);
    const char* action      = hash_map_get_value(
        &map->sourceToActions, &event->source, sizeof(InputEventSource));
    if (action != NULL)
    {
      printf("Action %s -> %f\n", action, event->value);
      hash_map_set_value_float(
          &map->actionValues, action, strlen(action) + 1, event->value);
    }
  }
}

float input_map_get_action_value(InputMap* map, char* action)
{
  return hash_map_get_value_float(
      &map->actionValues, action, strlen(action) + 1);
}

void input_map_destroy(InputMap* map)
{
  hash_map_destroy(&map->sourceToActions, NULL);
  hash_map_destroy(&map->actionValues, NULL);
}
