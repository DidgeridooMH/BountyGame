#pragma once

typedef struct Vec2
{
  union
  {
    struct
    {
      float x;
      float y;
    };
    float val[2];
  };
} Vec2;

typedef struct Vec3
{
  union
  {
    struct
    {
      float x;
      float y;
      float z;
    };
    float val[3];
  };
} Vec3;
