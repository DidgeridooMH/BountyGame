#pragma once

typedef struct InputMap InputMap;
typedef struct RenderInstance RenderInstance;

void update_camera_position(
    InputMap* map, RenderInstance* renderInstance, float deltaTime);
