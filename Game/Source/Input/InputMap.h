#pragma once

// Requirements
// 1. Read in keybinds from a config file.
// 6. Rumble support
// (7. Haptic support)
// (8. Trigger lock support)

#include <stdint.h>
#include <xinput.h>

#include "Otter/Util/AutoArray.h"
#include "Otter/Util/HashMap.h"

typedef enum InputSource : uint16_t
{
  INPUT_TYPE_MOUSE,
  INPUT_TYPE_KEYBOARD,
  INPUT_TYPE_CONTROLLER,
} InputSource;

typedef enum ControllerInputIndex
{
  CII_DPAD_UP,
  CII_DPAD_DOWN,
  CII_DPAD_LEFT,
  CII_DPAD_RIGHT,
  CII_START,
  CII_BACK,
  CII_LEFT_THUMB,
  CII_RIGHT_THUMB,
  CII_LEFT_SHOULDER,
  CII_RIGHT_SHOULDER,
  CII_A,
  CII_B,
  CII_X,
  CII_Y,
  CII_LEFT_TRIGGER,
  CII_RIGHT_TRIGGER,
  CII_LEFT_THUMB_X_NEG, // 16
  CII_LEFT_THUMB_X_POS,
  CII_LEFT_THUMB_Y_NEG,
  CII_LEFT_THUMB_Y_POS,
  CII_RIGHT_THUMB_X_NEG,
  CII_RIGHT_THUMB_X_POS,
  CII_RIGHT_THUMB_Y_NEG,
  CII_RIGHT_THUMB_Y_POS
} ControllerInputIndex;

typedef struct InputEventSource
{
  // @brief A type of input source, such as keyboard, mouse, or controller.
  InputSource source : 16;

  // @brief A keycode for keyboard input, a mouse button index for mouse input,
  // or a controller button index for controller input.
  uint16_t index : 16;
} InputEventSource;

typedef struct InputEvent
{
  // @brief Device and keycode for the event.
  InputEventSource source;

  // @brief Value 0.0 to 1.0 for the event action.
  float value;
} InputEvent;

typedef struct InputMap
{
  HashMap sourceToActions;
  HashMap actionValues;
  XINPUT_STATE previousControllerState[XUSER_MAX_COUNT];
} InputMap;

/**
 * @brief Create an input map.
 *
 * @param map The input map to create.
 * @return true if the input map was created successfully, false otherwise.
 */
bool input_map_create(InputMap* map);

/**
 * @brief Load key binds from a hashmap into an input map.
 *
 * @param map The input map to load the key binds into.
 * @param key_binds The key binds to load into the input map.
 */
void input_map_load_key_binds(InputMap* map, HashMap* key_binds);

/**
 * @brief Add an action to the input map.
 *
 * @param map The input map to add the action to.
 * @param source The source of the input event.
 * @param action The action to add.
 */
void input_map_add_action(InputMap* map, InputEventSource source, char* action);

/**
 * @brief Remove an action from the input map.
 *
 * @param map The input map to remove the action from.
 * @param source The source of the input event.
 */
void input_map_remove_action(InputMap* map, InputEventSource source);

/**
 * @brief Update the actions in the input map.
 *
 * @param map The input map to update.
 * @param inputs The input events to update the actions with.
 */
void input_map_update_actions(InputMap* map, AutoArray* inputs);

/**
 * @brief Update the controller actions in the input map.
 *
 * @param map The input map to update the controller actions with.
 */
void input_map_update_controller_actions(InputMap* map);

/**
 * @brief Get the value of an action in the input map.
 *
 * @param map The input map to get the action value from.
 * @param action The action to get the value of.
 * @return The value of the action.
 */
float input_map_get_action_value(InputMap* map, char* action);

/**
 * @brief Destroy an input map.
 *
 * @param map The input map to destroy.
 */
void input_map_destroy(InputMap* map);

