#include "Otter/Util/Math/Mat.h"

void mat4_identity(Mat4 matrix)
{
  memset(matrix, 0, sizeof(Mat4));
  matrix[0][0] = 1.0f;
  matrix[1][1] = 1.0f;
  matrix[2][2] = 1.0f;
  matrix[3][3] = 1.0f;
}

void mat4_multiply(Mat4 operand, Mat4 matrix)
{
  Mat4 result = {0};

  for (int i = 0; i < 4; ++i)
  {
    for (int j = 0; j < 4; ++j)
    {
      for (int k = 0; k < 4; ++k)
      {
        result[i][j] += matrix[i][k] * operand[k][j];
      }
    }
  }

  memcpy(matrix, result, sizeof(Mat4));
}

void mat4_translate(Mat4 matrix, float x, float y, float z)
{
  Mat4 translationMatrix;
  mat4_identity(translationMatrix);
  translationMatrix[3][0] = x;
  translationMatrix[3][1] = y;
  translationMatrix[3][2] = z;

  mat4_multiply(translationMatrix, matrix);
}

void mat4_scale(Mat4 matrix, float x, float y, float z)
{
  Mat4 scaleMatrix;
  mat4_identity(scaleMatrix);
  scaleMatrix[0][0] = x;
  scaleMatrix[1][1] = y;
  scaleMatrix[2][2] = z;

  mat4_multiply(scaleMatrix, matrix);
}

void mat4_rotate(Mat4 matrix, float roll, float pitch, float yaw)
{
  float sinAlpha = sinf(yaw);
  float cosAlpha = cosf(yaw);

  float sinBeta = sinf(pitch);
  float cosBeta = cosf(pitch);

  float sinGamma = sinf(roll);
  float cosGamma = cosf(roll);

  Mat4 rotationMatrix;
  mat4_identity(rotationMatrix);
  rotationMatrix[0][0] = cosBeta * cosGamma;
  rotationMatrix[0][1] = cosBeta * sinGamma;
  rotationMatrix[0][2] = -sinBeta;
  rotationMatrix[1][0] = sinAlpha * sinBeta * cosGamma - cosAlpha * sinGamma;
  rotationMatrix[1][1] = sinAlpha * sinBeta * sinGamma + cosAlpha * cosGamma;
  rotationMatrix[1][2] = sinAlpha * cosBeta;
  rotationMatrix[2][0] = cosAlpha * sinBeta * cosGamma + sinAlpha * sinGamma;
  rotationMatrix[2][1] = cosAlpha * sinBeta * sinGamma - sinAlpha * cosGamma;
  rotationMatrix[2][2] = cosAlpha * cosBeta;

  mat4_multiply(rotationMatrix, matrix);
}
