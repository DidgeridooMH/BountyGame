#include "Input/InputMap.h"

#include <xinput.h>

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

static void input_map_update_action(
    InputMap* map, InputEventSource source, float value)
{
  const char* action = hash_map_get_value(
      &map->sourceToActions, &source, sizeof(InputEventSource));
  if (action != NULL)
  {
    printf("Action %s -> %f\n", action, value);
    hash_map_set_value_float(
        &map->actionValues, action, strlen(action) + 1, value);
  }
}

void input_map_update_actions(InputMap* map, AutoArray* inputs)
{
  for (size_t i = 0; i < inputs->size; ++i)
  {
    const InputEvent* event = auto_array_get(inputs, i);
    input_map_update_action(map, event->source, event->value);
  }
}

static void input_map_update_button_bitmap(
    InputMap* map, WORD buttons, const ControllerInputIndex sources[])
{
  for (size_t i = 0; i < 16; ++i)
  {
    float value = (buttons & (1 << i)) ? 1.0f : 0.0f;
    input_map_update_action(
        map, (InputEventSource){INPUT_TYPE_CONTROLLER, sources[i]}, value);
  }
}

static void input_map_update_axis(InputMap* map, float value,
    ControllerInputIndex source, float maxValue, float threshold)
{
  input_map_update_action(map,
      (InputEventSource){INPUT_TYPE_CONTROLLER, source},
      (value > threshold) ? (value / maxValue) : 0.0f);
}

void input_map_update_controller_actions(InputMap* map)
{
  AutoArray inputs;
  auto_array_create(&inputs, sizeof(InputEvent));

  XINPUT_STATE state;
  for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
  {
    if (XInputGetState(i, &state) == ERROR_SUCCESS)
    {
      input_map_update_button_bitmap(map, state.Gamepad.wButtons,
          (ControllerInputIndex[]){CII_A, CII_B, CII_X, CII_Y,
              CII_LEFT_SHOULDER, CII_RIGHT_SHOULDER, CII_LEFT_THUMB,
              CII_RIGHT_THUMB, CII_BACK, CII_START, CII_DPAD_UP, CII_DPAD_DOWN,
              CII_DPAD_LEFT, CII_DPAD_RIGHT, CII_LEFT_TRIGGER,
              CII_RIGHT_TRIGGER});

      input_map_update_axis(map, (float) state.Gamepad.bLeftTrigger,
          CII_LEFT_TRIGGER, 127.0f, 0.0f);
      input_map_update_axis(map, (float) state.Gamepad.bRightTrigger,
          CII_RIGHT_TRIGGER, 127, 0.0f);

      input_map_update_axis(map, (float) -state.Gamepad.sThumbLX,
          CII_LEFT_THUMB_X_NEG, 32768, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
      input_map_update_axis(map, (float) state.Gamepad.sThumbLX,
          CII_LEFT_THUMB_X_POS, 32767, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
      input_map_update_axis(map, (float) -state.Gamepad.sThumbLY,
          CII_LEFT_THUMB_Y_NEG, 32768, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
      input_map_update_axis(map, (float) state.Gamepad.sThumbLY,
          CII_LEFT_THUMB_Y_POS, 32767, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

      input_map_update_axis(map, (float) -state.Gamepad.sThumbRX,
          CII_RIGHT_THUMB_X_NEG, 32768, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
      input_map_update_axis(map, (float) state.Gamepad.sThumbRX,
          CII_RIGHT_THUMB_X_POS, 32767, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
      input_map_update_axis(map, (float) -state.Gamepad.sThumbRY,
          CII_RIGHT_THUMB_Y_NEG, 32768, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
      input_map_update_axis(map, (float) state.Gamepad.sThumbRY,
          CII_RIGHT_THUMB_Y_POS, 32767, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
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
