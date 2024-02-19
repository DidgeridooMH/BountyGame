#pragma once

typedef struct PlayerInput
{
  union
  {
    struct
    {
      uint8_t up : 1;
      uint8_t down : 1;
      uint8_t left : 1;
      uint8_t right : 1;
    };
    uint8_t actions;
  };
} PlayerInput;
