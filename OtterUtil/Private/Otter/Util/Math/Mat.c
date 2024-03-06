#include "Otter/Util/Math/Mat.h"

void mat4_identity(Mat4 matrix)
{
  memset(matrix, 0, sizeof(Mat4));
  matrix[0].val[0] = 1.0f;
  matrix[1].val[1] = 1.0f;
  matrix[2].val[2] = 1.0f;
  matrix[3].val[3] = 1.0f;
}
