#include "Render/RenderSystem.h"

#include "Otter/ECS/EntityComponentMap.h"
#include "Otter/Math/Mat.h"
#include "Otter/Render/Mesh.h"
#include "Otter/Render/RenderInstance.h"

void render_mesh_system(Context* context, uint64_t entityId, void** components)
{
  Mesh** mesh         = (Mesh**) components[0];
  Material** material = (Material**) components[1];

  Entity* entity =
      entity_component_map_get_entity(context->entityComponentMap, entityId);
  Mat4 transform;
  mat4_identity(transform);
  transform_apply(transform, &entity->transform);

  render_instance_queue_mesh_draw(
      *mesh, *material, transform, context->renderInstance);
}
