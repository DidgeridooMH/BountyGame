namespace OtterEngine.Math
{
  using System;
  using System.Runtime.InteropServices;

  [StructLayout(LayoutKind.Sequential)]
  class Transform
  {
    public Vec3 position;
    public Vec3 rotation;
    public Vec3 scale;

    public Transform()
    {
      position = new Vec3(0, 0, 0);
      rotation = new Vec3(0, 0, 0);
      scale = new Vec3(1, 1, 1);
    }

    public Transform(Vec3 position, Vec3 rotation, Vec3 scale)
    {
      this.position = position;
      this.rotation = rotation;
      this.scale = scale;
    }
  }
}
