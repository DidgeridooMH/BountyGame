#pragma once

// Requirements
// 1. Read in keybinds from a config file.
// 6. Rumble support
// (7. Haptic support)
// (8. Trigger lock support)

#include "Otter/Util/HashMap.h"
#include <stdint.h>

typedef enum InputSource : uint16_t
{
  INPUT_TYPE_MOUSE,
  INPUT_TYPE_KEYBOARD,
  INPUT_TYPE_CONTROLLER,
} InputSource;

typedef struct InputEvent
{
  // @brief A type of input source, such as keyboard, mouse, or controller.
  InputSource source : 16;

  // @brief A keycode for keyboard input, a mouse button index for mouse input,
  // or a controller button index for controller input.
  uint16_t index : 16;
} InputEvent;

typedef struct InputBindings
{
  // @brief a mapping between input events and their actions.
  HashMap bindings;

  // @brief a mapping between action names and a list of registered callbacks.
  HashMap actions;
} InputBindings;

// TODO: As input information comes in we need to set the values in a list of
// actions to fire. Then, every frame we need to check the list of actions and
// fire the appropriate callbacks. This will ensure that we can't have two
// inputs on the same frame.
