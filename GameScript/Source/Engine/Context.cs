namespace OtterEngine
{
  using System;
  using System.Runtime.InteropServices;

  [StructLayout(LayoutKind.Sequential)]
  public struct Context
  {
    private IntPtr _inputMap;
    private IntPtr _renderInstance;
    public float deltaTime;
  }
}
