#include "Config/GameConfig.h"

#include "Otter/Config/Config.h"
#include "Otter/Util/File.h"
#include "Otter/Util/HashMap.h"
#include "Otter/Util/Log.h"

#define CONFIG_WIDTH            "width"
#define CONFIG_HEIGHT           "height"
#define CONFIG_SHADER_DIRECTORY "shaderDirectory"
#define CONFIG_SAMPLE_MODEL     "sampleModel"

bool game_config_parse(GameConfig* config, const char* filename)
{
  char* configStr = file_load(filename, NULL);
  if (configStr == NULL)
  {
    LOG_ERROR("Unable to find file %s", filename);
    return false;
  }

  HashMap configMap;
  if (!config_parse(&configMap, configStr))
  {
    free(configStr);
    LOG_ERROR("Could not parse configuration.");
    return false;
  }
  free(configStr);

  char* widthStr =
      hash_map_get_value(&configMap, CONFIG_WIDTH, strlen(CONFIG_WIDTH) + 1);
  char* heightStr =
      hash_map_get_value(&configMap, CONFIG_HEIGHT, strlen(CONFIG_HEIGHT) + 1);
  config->width  = widthStr != NULL ? atoi(widthStr) : 1920;
  config->height = heightStr != NULL ? atoi(heightStr) : 1080;
  LOG_DEBUG("Setting window to (%d, %d)", config->width, config->height);

  config->shaderDirectory = hash_map_get_value(
      &configMap, CONFIG_SHADER_DIRECTORY, strlen(CONFIG_SHADER_DIRECTORY) + 1);
  if (config->shaderDirectory == NULL)
  {
    LOG_ERROR("Shader directory not found in configuration.");
    hash_map_destroy(&configMap, free);
    return false;
  }
  config->shaderDirectory = _strdup(config->shaderDirectory);
  LOG_DEBUG("Setting shader directory to %s", config->shaderDirectory);

  config->sampleModel = hash_map_get_value(
      &configMap, CONFIG_SAMPLE_MODEL, strlen(CONFIG_SAMPLE_MODEL) + 1);
  if (config->sampleModel == NULL)
  {
    LOG_ERROR("Sample model not found in configuration.");
    hash_map_destroy(&configMap, free);
    return false;
  }
  config->sampleModel = _strdup(config->sampleModel);
  LOG_DEBUG("Setting sample model to %s", config->sampleModel);

  hash_map_destroy(&configMap, free);

  return true;
}

void game_config_destroy(GameConfig* config)
{
  free(config->shaderDirectory);
  free(config->sampleModel);
}
