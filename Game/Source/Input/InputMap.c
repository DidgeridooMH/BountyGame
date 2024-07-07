#include "Input/InputMap.h"

#include "Otter/Config/Config.h"
#include "Otter/Util/File.h"

bool input_map_create(InputMap* map)
{
  for (size_t i = 0; i < XUSER_MAX_COUNT; ++i)
  {
    heap_create(&map->rumbleQueue[i][0]);
    heap_create(&map->rumbleQueue[i][1]);
  }

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
  else if (definitionSize > 1 && definition[0] == 'M')
  {
    source->source = INPUT_TYPE_MOUSE;
    source->index  = atoi(&definition[1]);
  }
  else if (definitionSize > 1 && definition[0] == 'C')
  {
    source->source = INPUT_TYPE_CONTROLLER;
    source->index  = atoi(&definition[1]);
  }
  else
  {
    LOG_WARNING("Unknown input source %s", definition);
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

bool input_map_load_key_binds_from_file(InputMap* map, const char* path)
{
  LOG_DEBUG("Loading key binds from %s", path);

  HashMap keyBinds;
  char* keyBindStr = file_load(path, NULL);
  if (keyBindStr == NULL)
  {
    LOG_ERROR("Unable to find file %s", path);
    return false;
  }

  if (!config_parse(&keyBinds, keyBindStr))
  {
    LOG_ERROR("Could not parse key binds.");
    free(keyBindStr);
    return false;
  }

  free(keyBindStr);

  input_map_load_key_binds(map, &keyBinds);

  hash_map_destroy(&keyBinds, free);

  return true;
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

static void input_map_update_actions(InputMap* map, AutoArray* inputs)
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
    if (sources[i] != -1 && (mask & (1 << i)) > 0)
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

static void input_map_update_controller_actions(InputMap* map)
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
          (ControllerInputIndex[]){CII_DPAD_UP, CII_DPAD_DOWN, CII_DPAD_LEFT,
              CII_DPAD_RIGHT, CII_START, CII_BACK, CII_LEFT_THUMB,
              CII_RIGHT_THUMB, CII_LEFT_SHOULDER, CII_RIGHT_SHOULDER, -1, -1,
              CII_A, CII_B, CII_X, CII_Y},
          state.Gamepad.wButtons ^ previousState->Gamepad.wButtons);

      input_map_update_axis(map, (float) state.Gamepad.bLeftTrigger,
          CII_LEFT_TRIGGER, 255.0f, 0.0f,
          (float) previousState->Gamepad.bLeftTrigger);
      input_map_update_axis(map, (float) state.Gamepad.bRightTrigger,
          CII_RIGHT_TRIGGER, 255.0f, 0.0f,
          previousState->Gamepad.bRightTrigger);

      input_map_update_axis(map, (float) -state.Gamepad.sThumbLX,
          CII_LEFT_THUMB_X_NEG, 32768.0f, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,
          (float) -previousState->Gamepad.sThumbLX);
      input_map_update_axis(map, (float) state.Gamepad.sThumbLX,
          CII_LEFT_THUMB_X_POS, 32767.0f, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,
          (float) previousState->Gamepad.sThumbLX);
      input_map_update_axis(map, (float) -state.Gamepad.sThumbLY,
          CII_LEFT_THUMB_Y_NEG, 32768.0f, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,
          (float) -previousState->Gamepad.sThumbLY);
      input_map_update_axis(map, (float) state.Gamepad.sThumbLY,
          CII_LEFT_THUMB_Y_POS, 32767.0f, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,
          (float) previousState->Gamepad.sThumbLY);

      input_map_update_axis(map, (float) -state.Gamepad.sThumbRX,
          CII_RIGHT_THUMB_X_NEG, 32768.0f, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE,
          (float) -previousState->Gamepad.sThumbRX);
      input_map_update_axis(map, (float) state.Gamepad.sThumbRX,
          CII_RIGHT_THUMB_X_POS, 32767.0f, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE,
          (float) previousState->Gamepad.sThumbRX);
      input_map_update_axis(map, (float) -state.Gamepad.sThumbRY,
          CII_RIGHT_THUMB_Y_NEG, 32768.0f, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE,
          (float) -previousState->Gamepad.sThumbRY);
      input_map_update_axis(map, (float) state.Gamepad.sThumbRY,
          CII_RIGHT_THUMB_Y_POS, 32767.0f, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE,
          (float) previousState->Gamepad.sThumbRY);
    }

    map->previousControllerState[i] = state;
  }
}

void input_map_update(InputMap* map, AutoArray* inputs, float deltaTime)
{
  input_map_update_actions(map, inputs);
  input_map_update_controller_actions(map);

  for (DWORD userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex)
  {
    uint16_t motorStrength[2] = {0};
    for (size_t motorIndex = 0; motorIndex < 2; ++motorIndex)
    {
      // Decrease the time remaining for all rumble effects.
      for (size_t i = 0;
           i < map->rumbleQueue[userIndex][motorIndex].contents.size; ++i)
      {
        HeapElement* element = auto_array_get(
            &map->rumbleQueue[userIndex][motorIndex].contents, i);
        element->value -= deltaTime;
      }

      // Pop off any rumble effects that have expired.
      while (map->rumbleQueue[userIndex][motorIndex].contents.size > 0)
      {
        uint32_t strength;
        float timeRemaining;
        heap_top(&map->rumbleQueue[userIndex][motorIndex], &strength,
            &timeRemaining);

        if (timeRemaining > 0.0f)
        {
          motorStrength[motorIndex] = strength;
          break;
        }
        heap_pop(&map->rumbleQueue[userIndex][motorIndex]);
      }
    }

    XInputSetState(userIndex,
        &(XINPUT_VIBRATION){.wLeftMotorSpeed = motorStrength[RP_LOW_FREQUENCY],
            .wRightMotorSpeed = motorStrength[RP_HIGH_FREQUENCY]});
  }
}

float input_map_get_action_value(InputMap* map, char* action)
{
  return hash_map_get_value_float(
      &map->actionValues, action, strlen(action) + 1);
}

void input_map_queue_rumble_effect(InputMap* map, int controllerIndex,
    RumblePitch pitch, uint16_t strength, float duration)
{
  if (controllerIndex < XUSER_MAX_COUNT)
  {
    heap_push(&map->rumbleQueue[controllerIndex][pitch], strength, duration);
  }
}

void input_map_destroy(InputMap* map)
{
  for (size_t i = 0; i < XUSER_MAX_COUNT; ++i)
  {
    heap_destroy(&map->rumbleQueue[i][0]);
    heap_destroy(&map->rumbleQueue[i][1]);
  }
  hash_map_destroy(&map->sourceToActions, NULL);
  hash_map_destroy(&map->actionValues, NULL);
}

