#include "transform.h"
#include <string.h>
#include <math.h>

float vec3_mult(const vec3_t* left, const vec3_t* right)
{
  return ((left->vals[0] * right->vals[0])
          + (left->vals[1] * right->vals[1])
          + (left->vals[2] * right->vals[2]));
}

float vec2_mult(const vec2_t* left, const vec2_t* right)
{
  return ((left->vals[0] * right->vals[0])
          + (left->vals[1] * right->vals[1]));
}

float vec2_len(const vec2_t* vec)
{
  return sqrt((vec->vals[0] * vec->vals[0]) + (vec->vals[1] * vec->vals[1]));
}

vec2_t vec2_lerp(const vec2_t* v1, const vec2_t* v2, const float t)
{
  vec2_t result;
  result.vals[0] = ((1 - t) * v1->vals[0]) + (t * v2->vals[0]);
  result.vals[1] = ((1 - t) * v1->vals[1]) + (t * v2->vals[1]);
  return result;
}

void mat3_mult(const mat3_t* left, const mat3_t* right, mat3_t* result)
{
  vec3_t left_cols[3];
  vec3_t right_rows[3];

  for (int i = 0; i < 3; i++)
  {
    left_cols[i].vals[0] = left->vals[i];
    left_cols[i].vals[1] = left->vals[i + 3];
    left_cols[i].vals[2] = left->vals[i + 6];
  }

  for (int i = 0; i < 3; i++)
  {
    int row = (i * 3);
    right_rows[i].vals[0] = right->vals[row];
    right_rows[i].vals[1] = right->vals[row + 1];
    right_rows[i].vals[2] = right->vals[row + 2];
  }
  
  for (int x = 0; x < 3; x++)
  {
    for (int y = 0; y < 3; y++)
    {
      result->vals[x + (y * 3)] = vec3_mult(&left_cols[x], &right_rows[y]);
    }
  }
}

vec3_t mat3_vec3_mult(const mat3_t* left, const vec3_t* right)
{
  vec3_t row1 = { left->vals[0], left->vals[1], left->vals[2] };
  vec3_t row2 = { left->vals[3], left->vals[4], left->vals[5] };
  vec3_t row3 = { left->vals[6], left->vals[7], left->vals[8] };

  vec3_t result;
  result.vals[0] = vec3_mult(&row1, right);
  result.vals[1] = vec3_mult(&row2, right);
  result.vals[2] = vec3_mult(&row3, right);
  
  return result;
}

void mat3_translation(float x, float y, mat3_t* result)
{
  mat3_identity(result);
  result->vals[2] = x;
  result->vals[5] = y;
}

void mat3_rotation(float theta, mat3_t* result)
{
  mat3_identity(result);
  result->vals[0] = cos(theta);
  result->vals[1] = -sin(theta);
  result->vals[3] = sin(theta);
  result->vals[4] = cos(theta);
}

void mat3_scale(float hx, float hy, mat3_t* result)
{
  mat3_identity(result);
  result->vals[0] = hx;
  result->vals[4] = hy;
}

void mat3_shear(float hx, float hy, mat3_t* result)
{
  mat3_identity(result);
  result->vals[1] = hx;
  result->vals[3] = hy;
}

void mat3_reflect(mat3_t* result)
{
  mat3_identity(result);
  result->vals[0] = 0;
  result->vals[1] = 1;
  result->vals[4] = 0;
  result->vals[3] = 1;
}

void mat3_identity(mat3_t* result)
{
  memset((void*) result->vals, 0, sizeof(result->vals));
  for (int i = 0; i < 3; i++)
  {
    result->vals[i + (3 * i)] = 1;
  }
}
