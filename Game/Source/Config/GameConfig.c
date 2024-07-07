#include "Config/GameConfig.h"

#include "Otter/Config/Config.h"
#include "Otter/Util/File.h"
#include "Otter/Util/HashMap.h"
#include "Otter/Util/Log.h"

#define CONFIG_WIDTH  "width"
#define CONFIG_HEIGHT "height"

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

  hash_map_destroy(&configMap, free);

  return true;
}
