#pragma once

typedef struct InputMap InputMap;
typedef struct RenderInstance RenderInstance;
typedef struct EntityComponentMap EntityComponentMap;

typedef struct Context
{
  InputMap* inputMap;
  RenderInstance* renderInstance;
  EntityComponentMap* entityComponentMap;
  float deltaTime;
} Context;

