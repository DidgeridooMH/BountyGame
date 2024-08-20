namespace OtterEngine.Math
{
  using System;
  using System.Runtime.InteropServices;

  [StructLayout(LayoutKind.Sequential)]
  class Vec3
  {
    public float x;
    public float y;
    public float z;

    public Vec3(float x, float y, float z)
    {
      this.x = x;
      this.y = y;
      this.z = z;
    }

    public override string ToString()
    {
      return "(" + x + ", " + y + ", " + z + ")";
    }

    public float this[int index]
    {
      get
      {
        if (index < 0 || index > 2)
        {
          throw new IndexOutOfRangeException();
        }
        return index == 0 ? x : index == 1 ? y : z;
      }
      set
      {
        if (index < 0 || index > 2)
        {
          throw new IndexOutOfRangeException();
        }
        if (index == 0)
        {
          x = value;
        }
        else if (index == 1)
        {
          y = value;
        }
        else
        {
          z = value;
        }
      }
    }

    public static Vec3 operator +(Vec3 a, Vec3 b)
    {
      Vec3 result = new Vec3(a.x, a.y, a.z);
      vec3_add(result, b);
      return result;
    }

    public static Vec3 operator -(Vec3 a, Vec3 b)
    {
      Vec3 result = new Vec3(a.x, a.y, a.z);
      vec3_subtract(result, b);
      return result;
    }

    public static Vec3 operator *(Vec3 a, Vec3 b)
    {
      Vec3 result = new Vec3(a.x, a.y, a.z);
      vec3_multiply(result, b);
      return result;
    }

    public static Vec3 operator /(Vec3 a, Vec3 b)
    {
      Vec3 result = new Vec3(a.x, a.y, a.z);
      vec3_divide(result, b);
      return result;
    }

    public static Vec3 Normalize(Vec3 a)
    {
      Vec3 result = new Vec3(a.x, a.y, a.z);
      vec3_normalize(result);
      return result;
    }

    public static Vec3 Cross(Vec3 a, Vec3 b)
    {
      Vec3 result = new Vec3(a.x, a.y, a.z);
      vec3_cross(result, b);
      return result;
    }

    [DllImport("OtterMath", CallingConvention = CallingConvention.Cdecl)]
    private static extern void vec3_add(Vec3 a, Vec3 b);

    [DllImport("OtterMath", CallingConvention = CallingConvention.Cdecl)]
    private static extern void vec3_subtract(Vec3 a, Vec3 b);

    [DllImport("OtterMath", CallingConvention = CallingConvention.Cdecl)]
    private static extern void vec3_multiply(Vec3 a, Vec3 b);

    [DllImport("OtterMath", CallingConvention = CallingConvention.Cdecl)]
    private static extern void vec3_divide(Vec3 a, Vec3 b);

    [DllImport("OtterMath", CallingConvention = CallingConvention.Cdecl)]
    private static extern void vec3_normalize(Vec3 a);

    [DllImport("OtterMath", CallingConvention = CallingConvention.Cdecl)]
    private static extern void vec3_cross(Vec3 a, Vec3 b);

    [DllImport("OtterMath", CallingConvention = CallingConvention.Cdecl)]
    private static extern void vec3_dot(Vec3 a, Vec3 b);
  }
}

