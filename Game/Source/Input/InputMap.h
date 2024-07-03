#pragma once

// Requirements
// 1. Read in keybinds from a config file.
// 6. Rumble support
// (7. Haptic support)
// (8. Trigger lock support)

#include <stdint.h>

#include "Otter/Util/AutoArray.h"
#include "Otter/Util/HashMap.h"

typedef enum InputSource : uint16_t
{
  INPUT_TYPE_MOUSE,
  INPUT_TYPE_KEYBOARD,
  INPUT_TYPE_CONTROLLER,
} InputSource;

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
} InputMap;

/**
 * @brief Create an input map.
 *
 * @param map The input map to create.
 * @return true if the input map was created successfully, false otherwise.
 */
bool input_map_create(InputMap* map);

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

