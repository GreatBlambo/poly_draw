#pragma once

#include "gl_pixel_display.h"
#include "geom.h"

void draw_point(pixel_display_t* display, pixel_t color, int x, int y, unsigned int radius);

void clear_display(pixel_display_t* display, pixel_t color);

void draw_line(pixel_display_t* display, pixel_t color,
               int x1, int y1, int x2, int y2);

void draw_polygon_bounds(pixel_display_t* display, pixel_t color, polygon_t* poly);
void draw_polygon_points(pixel_display_t* display, pixel_t color, polygon_t* p, unsigned int radius);

void scan_fill(pixel_display_t* display, pixel_t color, polygon_t* p);
