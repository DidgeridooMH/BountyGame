#pragma once

#include "Otter/ECS/export.h"
#include "Otter/Math/Transform.h"
#include "Otter/Script/ScriptEngine.h"
#include "Otter/Util/Array/SparseAutoArray.h"
#include "Otter/Util/HashMap.h"

typedef struct Entity
{
  uint64_t id;
  HashMap componentIndices;
  SparseAutoArray scripts;
  Transform transform;
} Entity;

OTTERECS_API bool entity_create(Entity* entity, uint64_t id);

OTTERECS_API void entity_destroy(Entity* entity, ScriptEngine* scriptEngine);

OTTERECS_API bool entity_add_script(Entity* entity, const char* script,
    ScriptEngine* scriptEngine, uint64_t* scriptId);

OTTERECS_API void entity_remove_script(
    Entity* entity, uint64_t scriptId, ScriptEngine* scriptEngine);

OTTERECS_API void entity_run_update(Entity* entity, uint64_t entityId,
    ScriptEngine* scriptEngine, void* context);

OTTERECS_API Transform* entity_get_transform(Entity* entity);
