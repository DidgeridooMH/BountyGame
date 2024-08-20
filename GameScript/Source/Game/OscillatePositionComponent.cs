namespace BountyGame
{
  using System;
  using OtterEngine;
  using OtterEngine.ECS;
  using OtterEngine.Math;

  class OscillatePositionComponent : BaseComponent
  {
    public override void OnUpdate(Context context)
    {
      Transform transform = context.GetTransform(this.entity);
      Vec3 velocity = context.GetComponent<Vec3>(this.entity, ComponentType.Velocity);

      if (transform.position.x > 16.0f)
      {
        velocity.x = -1.0f;
      }
      else if (transform.position.x < -16.0f)
      {
        velocity.x = 1.0f;
      }

      context.SetComponent(this.entity, ComponentType.Velocity, velocity);
    }
  }
}
