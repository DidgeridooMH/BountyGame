#include "Input/InputMap.h"

bool input_map_create(InputMap* map)
{
  return hash_map_create(&map->sourceToActions, HASH_MAP_DEFAULT_BUCKETS,
             HASH_MAP_DEFAULT_COEF)
      && hash_map_create(
          &map->actionValues, HASH_MAP_DEFAULT_BUCKETS, HASH_MAP_DEFAULT_COEF);
}

static bool input_map_convert_definition_to_source(
    char* definition, size_t definitionSize, InputEventSource* source)
{
  if (definitionSize == 1)
  {
    source->source = INPUT_TYPE_KEYBOARD;
    source->index  = definition[0];
  }
  else if (definitionSize > 1 & definition[0] == 'M')
  {
    source->source = INPUT_TYPE_MOUSE;
    source->index  = atoi(&definition[1]);
  }
  else if (definitionSize > 1 & definition[0] == 'C')
  {
    source->source = INPUT_TYPE_CONTROLLER;
    source->index  = atoi(&definition[1]);
  }
  else
  {
    fprintf(stderr, "Unknown input source %s\n", definition);
    return false;
  }
  return true;
}

static void input_map_add_action_from_key_bind(
    char* action, size_t actionLength, char* input, InputMap* map)
{
  (void) actionLength;

  size_t cursor = 0;
  while (true)
  {
    InputEventSource source;
    if ((input[cursor] == ',' || input[cursor] == '\0')
        && input_map_convert_definition_to_source(input, cursor, &source))
    {
      input_map_add_action(map, source, action);
    }

    if (input[cursor] == ',')
    {
      input  = &input[cursor + 1];
      cursor = 0;
    }
    else if (input[cursor] == '\0')
    {
      return;
    }

    cursor += 1;
  }
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

static void input_map_update_button_bitmap(InputMap* map, WORD buttons,
    const ControllerInputIndex sources[], DWORD mask)
{
  for (size_t i = 0; i < 16; ++i)
  {
    if ((mask & (1 << i)) > 0)
    {
      float value = (buttons & (1 << i)) ? 1.0f : 0.0f;
      input_map_update_action(
          map, (InputEventSource){INPUT_TYPE_CONTROLLER, sources[i]}, value);
    }
  }
}

static void input_map_update_axis(InputMap* map, float value,
    ControllerInputIndex source, float maxValue, float threshold,
    float previousValue)
{
  value = (value > threshold) ? (value / maxValue) : 0.0f;
  previousValue =
      (previousValue > threshold) ? (previousValue / maxValue) : 0.0f;
  if (fabsf(value - previousValue) > 0.001f)
  {
    input_map_update_action(
        map, (InputEventSource){INPUT_TYPE_CONTROLLER, source}, value);
  }
}

void input_map_update_controller_actions(InputMap* map)
{
  AutoArray inputs;
  auto_array_create(&inputs, sizeof(InputEvent));

  XINPUT_STATE state;
  for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
  {
    const XINPUT_STATE* previousState = &map->previousControllerState[i];
    if (XInputGetState(i, &state) == ERROR_SUCCESS
        && state.dwPacketNumber != previousState->dwPacketNumber)
    {
      input_map_update_button_bitmap(map, state.Gamepad.wButtons,
          (ControllerInputIndex[]){CII_A, CII_B, CII_X, CII_Y,
              CII_LEFT_SHOULDER, CII_RIGHT_SHOULDER, CII_LEFT_THUMB,
              CII_RIGHT_THUMB, CII_BACK, CII_START, CII_DPAD_UP, CII_DPAD_DOWN,
              CII_DPAD_LEFT, CII_DPAD_RIGHT, CII_LEFT_TRIGGER,
              CII_RIGHT_TRIGGER},
          state.Gamepad.wButtons ^ previousState->Gamepad.wButtons);

      input_map_update_axis(map, state.Gamepad.bLeftTrigger, CII_LEFT_TRIGGER,
          127.0f, 0.0f, previousState->Gamepad.bLeftTrigger);
      input_map_update_axis(map, (float) state.Gamepad.bRightTrigger,
          CII_RIGHT_TRIGGER, 127.0f, 0.0f,
          previousState->Gamepad.bRightTrigger);

      input_map_update_axis(map, -state.Gamepad.sThumbLX, CII_LEFT_THUMB_X_NEG,
          32768, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,
          -previousState->Gamepad.sThumbLX);
      input_map_update_axis(map, state.Gamepad.sThumbLX, CII_LEFT_THUMB_X_POS,
          32767, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,
          previousState->Gamepad.sThumbLX);
      input_map_update_axis(map, -state.Gamepad.sThumbLY, CII_LEFT_THUMB_Y_NEG,
          32768, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,
          -previousState->Gamepad.sThumbLY);
      input_map_update_axis(map, state.Gamepad.sThumbLY, CII_LEFT_THUMB_Y_POS,
          32767, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,
          previousState->Gamepad.sThumbLY);

      input_map_update_axis(map, -state.Gamepad.sThumbRX, CII_RIGHT_THUMB_X_NEG,
          32768, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE,
          -previousState->Gamepad.sThumbRX);
      input_map_update_axis(map, state.Gamepad.sThumbRX, CII_RIGHT_THUMB_X_POS,
          32767, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE,
          previousState->Gamepad.sThumbRX);
      input_map_update_axis(map, -state.Gamepad.sThumbRY, CII_RIGHT_THUMB_Y_NEG,
          32768, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE,
          -previousState->Gamepad.sThumbRY);
      input_map_update_axis(map, state.Gamepad.sThumbRY, CII_RIGHT_THUMB_Y_POS,
          32767, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE,
          previousState->Gamepad.sThumbRY);
    }

    map->previousControllerState[i] = state;
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

