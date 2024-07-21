#include "Otter/Render/RenderQueue.h"

int render_command_compare(const RenderCommand* a, const RenderCommand* b)
{
  if (a->material < b->material)
  {
    return -1;
  }
  else if (a->material > b->material)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}
