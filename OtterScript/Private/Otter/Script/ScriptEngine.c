#include "Otter/Script/ScriptEngine.h"

#include "Otter/Util/File.h"
#include "Otter/Util/Log.h"

void script_engine_init(ScriptEngine* engine, const char* gameAssemblyName)
{
  char gameAssemblyPath[MAX_PATH];
  file_get_executable_path(gameAssemblyPath, _countof(gameAssemblyPath));
  strcat_s(gameAssemblyPath, _countof(gameAssemblyPath), gameAssemblyName);

  engine->domain = mono_jit_init("Otter");
  engine->assembly =
      mono_domain_assembly_open(engine->domain, gameAssemblyPath);
  if (!engine->assembly)
  {
    LOG_ERROR("Unable to load game assembly");
    return;
  }
  LOG_DEBUG("Game assembly loaded");
}

void script_engine_shutdown(ScriptEngine* engine)
{
  mono_jit_cleanup(engine->domain);
}

bool script_engine_create_component(ScriptEngine* engine,
    const char* componentName, uint32_t* component, uint64_t entity)
{
  MonoImage* image = mono_assembly_get_image(engine->assembly);
  MonoClass* componentClass =
      mono_class_from_name(image, GAME_SCRIPT_NAMESPACE, componentName);

  if (!componentClass)
  {
    LOG_ERROR(
        "Unable to find class %s.%s", GAME_SCRIPT_NAMESPACE, componentName);
    return false;
  }

  MonoObject* componentObject = mono_object_new(engine->domain, componentClass);
  mono_runtime_object_init(componentObject);

  MonoClassField* entityField =
      mono_class_get_field_from_name(componentClass, "entity");
  if (!entityField)
  {
    LOG_ERROR("Unable to find field: entity");
    return false;
  }
  mono_field_set_value(componentObject, entityField, &entity);

  *component = mono_gchandle_new(componentObject, false);

  return true;
}

void script_engine_destroy_component(ScriptEngine* engine, uint32_t component)
{
  mono_gchandle_free(component);
}

void script_engine_run_update(
    ScriptEngine* scriptEngine, uint32_t scriptComponent, void* context)
{
  MonoObject* scriptObject = mono_gchandle_get_target(scriptComponent);
  if (!scriptObject)
  {
    LOG_ERROR("Script component is null");
    return;
  }

  MonoImage* image = mono_assembly_get_image(scriptEngine->assembly);
  if (!image)
  {
    LOG_ERROR("Failed to get image from assembly");
    return;
  }

  MonoClass* interfaceClass =
      mono_class_from_name(image, ENGINE_NAMESPACE, "IComponent");
  if (!interfaceClass)
  {
    LOG_ERROR("Failed to get interface class: IComponent");
    return;
  }

  MonoMethod* interfaceMethod =
      mono_class_get_method_from_name(interfaceClass, "OnUpdate", 1);
  if (!interfaceMethod)
  {
    LOG_ERROR("Failed to get interface method: OnUpdate");
    return;
  }

  MonoMethod* updateMethod =
      mono_object_get_virtual_method(scriptObject, interfaceMethod);
  if (!updateMethod)
  {
    LOG_ERROR("Failed to get virtual method: OnUpdate");
    return;
  }

  mono_runtime_invoke(updateMethod, scriptObject, (void* [1]){context}, NULL);
}

