namespace OtterEngine
{
  using OtterEngine.Math;

  internal abstract class BaseComponent : IComponent
  {
    public ulong entity;

    public abstract void OnUpdate(Context context);
  }
}
