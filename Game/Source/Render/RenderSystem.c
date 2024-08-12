#include "Render/RenderSystem.h"

void render_mesh_system(Context* context, uint64_t entity, void** components)
{
  Mat4* transform     = (Mat4*) components[0];
  Mesh** mesh         = (Mesh**) components[1];
  Material** material = (Material**) components[2];

  render_instance_queue_mesh_draw(
      *mesh, *material, *transform, context->renderInstance);
}
