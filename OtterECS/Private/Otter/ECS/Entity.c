#include "Otter/ECS/Entity.h"

bool entity_create(Entity* entity, uint64_t id)
{
  if (!hash_map_create(&entity->componentIndices, HASH_MAP_DEFAULT_BUCKETS,
          HASH_MAP_DEFAULT_COEF))
  {
    return false;
  }
  sparse_auto_array_create(&entity->scripts, sizeof(uint32_t));
  transform_identity(&entity->transform);
  entity->id = id;

  return true;
}

void entity_destroy(Entity* entity, ScriptEngine* scriptEngine)
{
  hash_map_destroy(&entity->componentIndices, NULL);

  for (uint64_t i = 0; i < entity->scripts.components.size; ++i)
  {
    if (bit_map_get(&entity->scripts.usedMask, i))
    {
      entity_remove_script(entity, i, scriptEngine);
    }
  }

  sparse_auto_array_destroy(&entity->scripts);
}

bool entity_add_script(Entity* entity, const char* script,
    ScriptEngine* scriptEngine, uint64_t* scriptId)
{
  *scriptId = sparse_auto_array_allocate(&entity->scripts);
  uint32_t* scriptHandle =
      (uint32_t*) sparse_auto_array_get(&entity->scripts, *scriptId);

  if (!script_engine_create_component(
          scriptEngine, script, scriptHandle, entity->id))
  {
    sparse_auto_array_deallocate(&entity->scripts, *scriptId);
    return false;
  }

  return true;
}

void entity_remove_script(
    Entity* entity, uint64_t scriptId, ScriptEngine* scriptEngine)
{
  uint32_t* scriptHandle =
      (uint32_t*) sparse_auto_array_get(&entity->scripts, scriptId);
  script_engine_destroy_component(scriptEngine, *scriptHandle);
  sparse_auto_array_deallocate(&entity->scripts, scriptId);
}

void entity_run_update(Entity* entity, uint64_t entityId,
    ScriptEngine* scriptEngine, void* context)
{
  for (uint64_t i = 0; i < entity->scripts.components.size; ++i)
  {
    if (bit_map_get(&entity->scripts.usedMask, i))
    {
      uint32_t* scriptHandle =
          (uint32_t*) sparse_auto_array_get(&entity->scripts, i);
      script_engine_run_update(scriptEngine, *scriptHandle, context);
    }
  }
}

Transform* entity_get_transform(Entity* entity)
{
  return &entity->transform;
}

