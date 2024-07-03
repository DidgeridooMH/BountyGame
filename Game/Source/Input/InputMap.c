#include "Input/InputMap.h"

bool input_map_create(InputMap* map)
{
  return hash_map_create(&map->sourceToActions, HASH_MAP_DEFAULT_BUCKETS,
             HASH_MAP_DEFAULT_COEF)
      && hash_map_create(
          &map->actionValues, HASH_MAP_DEFAULT_BUCKETS, HASH_MAP_DEFAULT_COEF);
}

void input_map_add_action(InputMap* map, InputEventSource source, char* action)
{
  hash_map_set_value(
      &map->sourceToActions, &source, sizeof(InputEventSource), action);
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
