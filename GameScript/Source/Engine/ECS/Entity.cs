namespace OtterEngine.ECS
{
  using System;
  using System.Runtime.InteropServices;
  using OtterEngine.Math;

  internal class Entity
  {
    [DllImport("OtterECS", CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr entity_get_transform(IntPtr entity);
  }
}
