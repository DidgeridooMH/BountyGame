namespace BountyGame
{
  using System;
  using OtterEngine;

  public class AdderComponent : IComponent
  {
    private int _value;

    AdderComponent()
    {
      _value = 0;
    }

    public void OnUpdate(Context context)
    {
      _value++;
    }
  }
}
