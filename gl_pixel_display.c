#include "gl_pixel_display.h"

void create_pixel_display(pixel_display_t* display, size_t w, size_t h)
{
  glGenTextures(1, &display->tex);
  glGenBuffers(2, display->pbo);

  // Double buffered pbo init
  for (int i = 0; i < NUM_PIX_BUFFERS; i++)
  {
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, display->pbo[i]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeof(pixel_t) * w * h, 0, GL_STREAM_DRAW);
  }

  glBindTexture(GL_TEXTURE_2D, display->tex);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  // Init buff counter
  display->current_buff = 0;

  display->w = w;
  display->h = h;

  display->buf = NULL;
}

void delete_pixel_display(pixel_display_t* display)
{
  glDeleteTextures(1, &display->tex);
  glDeleteBuffers(2, display->pbo);
}

void pixel_display_fill_start(pixel_display_t* display)
{
  int index = (++display->current_buff) % NUM_PIX_BUFFERS;
  int next_index = (index + 1) % NUM_PIX_BUFFERS;
  
  glBindTexture(GL_TEXTURE_2D, display->tex);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, display->pbo[index]);

  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, display->w, display->h,
                  GL_BGRA, GL_UNSIGNED_BYTE, 0);

  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, display->pbo[next_index]);
  glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeof(pixel_t) * display->w * display->h, 0, GL_STREAM_DRAW);

  display->buf = (pixel_t*) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
  
  // Possibly handle null buf
}

void pixel_display_fill_end(pixel_display_t* display)
{
  glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  display->buf = NULL;
}
