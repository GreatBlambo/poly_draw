#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>


#pragma pack(push, 1)
typedef struct
{
  GLubyte b;
  GLubyte g;
  GLubyte r;
  GLubyte a;
} pixel_t;
#pragma pack(pop)

#define NUM_PIX_BUFFERS 2

typedef struct
{
  size_t w;
  size_t h;
  
  GLuint pbo[NUM_PIX_BUFFERS];
  int current_buff;
  GLuint tex;

  pixel_t* buf;
} pixel_display_t;

void create_pixel_display(pixel_display_t* display, size_t w, size_t h);

void delete_pixel_display(pixel_display_t* display);

void pixel_display_fill_start(pixel_display_t* display);

void pixel_display_fill_end(pixel_display_t* display);
