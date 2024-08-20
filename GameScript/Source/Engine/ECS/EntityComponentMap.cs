namespace OtterEngine.ECS
{
  using System;
  using System.Runtime.InteropServices;

  internal class EntityComponentMap
  {
    [DllImport("OtterECS", CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr entity_component_map_get_component(IntPtr map, ulong entity, ulong component);

    [DllImport("OtterECS", CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr entity_component_map_get_entity(IntPtr map, ulong entity);
  }
}
