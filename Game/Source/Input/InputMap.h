#pragma once

#include <stdint.h>
#include <xinput.h>

#include "Otter/Util/Array/AutoArray.h"
#include "Otter/Util/HashMap.h"
#include "Otter/Util/Heap.h"

#define DEFAULT_KEY_BINDS_PATH "Config/keybinds.ini"

// TODO: Port to GameInput.
// TODO: Need key values for LSHIFT and other non-character keys.

/** @brief The type of input source. */
typedef enum InputSource
{
  INPUT_TYPE_MOUSE,
  INPUT_TYPE_KEYBOARD,
  INPUT_TYPE_CONTROLLER,
} InputSource;

/** @brief The index of a controller input. */
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
  CII_LEFT_THUMB_X_NEG,
  CII_LEFT_THUMB_X_POS,
  CII_LEFT_THUMB_Y_NEG,
  CII_LEFT_THUMB_Y_POS,
  CII_RIGHT_THUMB_X_NEG,
  CII_RIGHT_THUMB_X_POS,
  CII_RIGHT_THUMB_Y_NEG,
  CII_RIGHT_THUMB_Y_POS
} ControllerInputIndex;

#pragma pack(push, 1)
typedef struct InputEventSource
{
  // @brief A type of input source, such as keyboard, mouse, or controller.
  InputSource source;

  // @brief A keycode for keyboard input, a mouse button index for mouse input,
  // or a controller button index for controller input.
  uint32_t index;
} InputEventSource;
#pragma pack(pop)

typedef struct InputEvent
{
  // @brief Device and keycode for the event.
  InputEventSource source;

  // @brief Value 0.0 to 1.0 for the event action.
  float value;
} InputEvent;

typedef enum RumblePitch
{
  RP_LOW_FREQUENCY  = 0,
  RP_HIGH_FREQUENCY = 1
} RumblePitch;

typedef struct InputMap
{
  HashMap sourceToActions;
  HashMap actionValues;
  XINPUT_STATE previousControllerState[XUSER_MAX_COUNT];
  Heap rumbleQueue[XUSER_MAX_COUNT][2];
  XINPUT_VIBRATION rumbleState[XUSER_MAX_COUNT];
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
 * @brief Load key binds from a file into an input map.
 *
 * @param map The input map to load the key binds into.
 * @param path The path to the file to load the key binds from.
 * @return true if the key binds were loaded successfully, false otherwise.
 */
bool input_map_load_key_binds_from_file(InputMap* map, const char* path);

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
 * @brief Update the input map with `inputs` and set the vibration state.
 *
 * @param map The input map to update.
 * @param inputs The inputs to update the input map with.
 * @param deltaTime The time since the last frame.
 */
void input_map_update(InputMap* map, AutoArray* inputs, float deltaTime);

/**
 * @brief Get the value of an action in the input map.
 *
 * @param map The input map to get the action value from.
 * @param action The action to get the value of.
 * @return The value of the action.
 */
float input_map_get_action_value(InputMap* map, char* action);

/**
 * @brief Queue a rumble effect for a controller.
 *
 * @param map The input map to queue the rumble effect on.
 * @param controllerIndex The index of the controller to rumble.
 * @param pitch The pitch of the rumble effect.
 * @param strength The strength of the rumble effect.
 * @param duration The duration of the rumble effect in seconds.
 */
void input_map_queue_rumble_effect(InputMap* map, int controllerIndex,
    RumblePitch pitch, uint16_t strength, float duration);

/**
 * @brief Destroy an input map.
 *
 * @param map The input map to destroy.
 */
void input_map_destroy(InputMap* map);
