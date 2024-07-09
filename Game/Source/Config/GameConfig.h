#pragma once

#define DEFAULT_GAME_CONFIG_PATH "Config/config.ini"

typedef struct GameConfig
{
  int width;
  int height;
  char* shaderDirectory;
  char* sampleModel;
} GameConfig;

bool game_config_parse(GameConfig* config, const char* filename);

void game_config_destroy(GameConfig* config);
