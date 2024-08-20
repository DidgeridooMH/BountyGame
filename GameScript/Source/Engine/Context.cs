namespace OtterEngine
{
  using System;
  using System.Runtime.InteropServices;
  using OtterEngine.ECS;
  using OtterEngine.Math;

  [StructLayout(LayoutKind.Sequential)]
  struct Context
  {
    private IntPtr _inputMap;
    private IntPtr _renderInstance;
    private IntPtr _entityMap;
    public float deltaTime;

    // TODO: Write your own marshalling code for this.
    public T GetComponent<T>(ulong entity, ComponentType component)
    {
      IntPtr componentPtr = EntityComponentMap.entity_component_map_get_component(_entityMap, entity, (ulong)component);
      return Marshal.PtrToStructure<T>(componentPtr);
    }

    public void SetComponent<T>(ulong entity, ComponentType component, T value)
    {
      IntPtr componentPtr = EntityComponentMap.entity_component_map_get_component(_entityMap, entity, (ulong)component);
      Marshal.StructureToPtr(value, componentPtr, false);
    }

    public Transform GetTransform(ulong entity)
    {
      IntPtr entityPtr = EntityComponentMap.entity_component_map_get_entity(_entityMap, entity);
      IntPtr entityTransform = Entity.entity_get_transform(entityPtr);
      return Marshal.PtrToStructure<Transform>(entityTransform);
    }
  }
}
