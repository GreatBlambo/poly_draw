#pragma once

typedef struct
{
  float vals[2];
} vec2_t;

typedef struct
{
  float vals[3];
} vec3_t;

typedef struct
{
  float vals[9];
} mat3_t;

float vec3_mult(const vec3_t* left, const vec3_t* right);

float vec2_mult(const vec2_t* left, const vec2_t* right);

float vec2_len(const vec2_t* vec);

vec2_t vec2_lerp(const vec2_t* v1, const vec2_t* v2, const float t);

void mat3_mult(const mat3_t* left, const mat3_t* right, mat3_t* result);

vec3_t mat3_vec3_mult(const mat3_t* left, const vec3_t* right);

void mat3_translation(float x, float y, mat3_t* result);

void mat3_rotation(float theta, mat3_t* result);

void mat3_scale(float hx, float hy, mat3_t* result);

void mat3_shear(float hx, float hy, mat3_t* result);

void mat3_reflect(mat3_t* result);

void mat3_identity(mat3_t* result);
